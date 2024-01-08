#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "lambda.hpp"

struct Location {
    Location() {}
    Location(size_t lno, size_t pos, size_t len) : lno(lno), pos(pos), len(len) {}

    std::string string(const std::vector<std::string>& lines) const { return lines[lno].substr(pos, len); }

    size_t lno, pos, len;
};

using TokenMat = std::vector<std::vector<Location>>;

class OldParseError {
  public:
    OldParseError(const std::vector<std::string>& lines, const std::string& msg, size_t lno, size_t pos, size_t len = 1) : srcname("unnamed"), msg(msg), lines(lines), loc(lno, pos, len) {}
    OldParseError(const std::string& srcname, const std::vector<std::string>& lines, const std::string& msg, size_t lno, size_t pos, size_t len = 1) : srcname(srcname), msg(msg), lines(lines), loc(lno, pos, len) {}

    OldParseError(const std::vector<std::string>& lines, const std::string& msg, const Location& loc) : srcname("unnamed"), msg(msg), lines(lines), loc(loc) {}
    OldParseError(const std::string& srcname, const std::vector<std::string>& lines, const std::string& msg, const Location& loc) : srcname(srcname), msg(msg), lines(lines), loc(loc) {}

    virtual void puterror(std::ostream& os = std::cerr) {
        os << srcname << ":" << loc.lno + 1 << ":" << loc.pos + 1 << ": " << msg << std::endl;
        std::string lnostr = std::to_string(loc.lno + 1) + " ";
        os << lnostr << "| " << lines[loc.lno] << std::endl;
        os << std::string(lnostr.size(), ' ') << "| " << std::string(loc.pos, ' ') << "^" << std::string(loc.len - 1, '~') << std::endl;
    }

    std::string srcname;
    std::string msg;
    const std::vector<std::string>& lines;
    Location loc;
};

class OldParseErrorWithNote : public OldParseError {
  public:
    OldParseErrorWithNote(const OldParseError& error, const OldParseError& note) : OldParseError(error), note(note) {}

    void puterror(std::ostream& os = std::cerr) override {
        static_cast<OldParseError>(*this).puterror(os);
        note.puterror(os);
    }
    OldParseError note;
};

class LambdaError {
  public:
    LambdaError(const std::string& msg, size_t pos, size_t len) : msg(msg), pos(pos), len(len) {}
    std::string msg;
    size_t pos, len;
};

std::shared_ptr<Term> parse_lambda(const std::vector<std::string>& lines, const TokenMat& tokenmat, int& row, int& col, size_t pos = 0) {
    const std::string name_syms("-_.");

    unused(lines, tokenmat, row, col, pos);
    // auto getchar = [&lines, &tokenmat, &row, &col, &pos]() {
    //     return tokenmat[row][col].string(lines)[pos];
    // };

    // if (isalpha(expr[pos])) {
    //     size_t tail = pos;
    //     while (tail + 1 < end && (isalnum(expr[tail]) || name_syms.find(expr[tail]) != std::string::npos)) ++tail;
    //     if (tail == pos) {
    //         if (tail + 1 < end) {
    //             throw LambdaError("unknown leading tokens", tail + 1, end - tail - 1);
    //         }
    //         return std::make_shared<Variable>(expr[tail]);
    //     }
    //     else {
    //         // constant
    //         std::string cname = expr.substr(pos, tail - pos + 1);
    //         std::cerr << "parse_lambda/cname = " << cname << std::endl;
    //     }
    // }

    return std::make_shared<Star>();
}

std::shared_ptr<Definition> parse_def(const std::vector<std::string>& lines, const TokenMat& tokenmat) {
    std::cerr << "parse_def: tokenmat..." << std::endl;
    for (int i = 0; i < (int)tokenmat.size(); ++i) {
        std::cerr << "line " << i << ":";
        for (int j = 0; j < (int)tokenmat[i].size(); ++j) std::cerr << " [" << tokenmat[i][j].string(lines) << "],";
        std::cerr << std::endl;
    }

    int i = 0, j = 0;
    auto incr = [&tokenmat](int& a, int& b) -> void {
        b + 1 < (int)tokenmat[a].size() ? ++b : ++a, b = 0;
    };
    auto token = [&lines, &tokenmat](int i, int j) -> std::string {
        return tokenmat[i][j].string(lines);
    };
    // line 0: header "def2"
    if (token(i, j) != "def2") throw OldParseError(lines, "(This is a bug. Please report with your input) header is not \"def2\"", tokenmat[i][j]);
    incr(i, j);
    // line 1: # of variables (N)
    size_t num_vars;
    // emptyness check
    try {
        num_vars = std::stoi(token(i, j));
    } catch (const std::invalid_argument& e) {
        throw OldParseError(lines, "failed to read a number from this token", tokenmat[i][j]);
    }
    std::cerr << "#vars = " << num_vars << std::endl;
    incr(i, j);
    std::vector<std::shared_ptr<Typed<Variable>>> vars;
    std::vector<std::shared_ptr<Term>> types;
    // line 2 ... (2*N + 1): context; pairs of variable and its type (lambda)
    for (size_t k = 0; k < num_vars; ++k) {
        // var
        int i0 = i, j0 = j;
        std::shared_ptr<Term> variable = parse_lambda(lines, tokenmat, i, j);
        if (!isTermA<Kind::Variable>(variable)) {
            throw OldParseError(lines, "expected variable (got " + to_string(variable->kind()) + ")", tokenmat[i0][j0]);
        }
        // type
        if (token(i, j) == ":") incr(i, j);
        std::string expr("");
        for (int orig_i = i; orig_i == i; incr(i, j)) expr += token(i, j);
        std::shared_ptr<Term> texpr;
        // texpr = parse_lambda(expr);
        texpr = parse_lambda(lines, tokenmat, i, j);
        vars.emplace_back(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(variable), texpr));
        types.emplace_back(texpr);
    }
    // line 2*N + 2: name of constant
    std::string cname = token(i, j);
    incr(i, j);
    // line 2*N + 3: proof (lambda)
    std::shared_ptr<Term> proof(parse_lambda(lines, tokenmat, i, j));
    // line 2*N + 4: proposition (lambda)
    std::shared_ptr<Term> prop(parse_lambda(lines, tokenmat, i, j));
    // line 2*N + 5: footer "edef2"
    if (token(i, j) != "edef2") throw OldParseError(lines, "(This is a bug. Please report with your input) footer is not \"edef2\"", tokenmat[i][j]);

    std::shared_ptr<Context> context = std::make_shared<Context>(vars);
    std::shared_ptr<Constant> constant = std::make_shared<Constant>(cname, types);
    return std::make_shared<Definition>(context, constant, proof, prop);
}

std::shared_ptr<Environment> parse(const std::vector<std::string>& lines) {
    std::vector<std::shared_ptr<Definition>> env;

    bool eof = false;
    bool comm = false;  // in a "/*" - "*/" clause
    int def_begin = -1;

    // last comms begin
    int last_comms_begin_lno = -1, last_comms_begin_pos = -1;

    Location last_def2;

    TokenMat tokenss;

    const std::string spaces(" \t\r\n");
    int token_head = -1;
    for (size_t lno = 0; lno < lines.size(); ++lno) {
        const std::string& str = lines[lno];
        std::vector<Location> tokens;
        int m = str.size();
        const std::string spaces(" \t\r\n");
        int lazpos = 0;
        for (int pos = 0; pos < m; ++pos) {
            bool check_token_tail = false;
            if (comm) {
                if (str.substr(pos, 2) == "*/") {
                    comm = false;
                    last_comms_begin_lno = -1;
                    last_comms_begin_pos = -1;
                    lazpos = 1;
                }
            } else {
                if (str.substr(pos, 2) == "/*") {
                    check_token_tail = true;
                    comm = true;
                    last_comms_begin_lno = lno;
                    last_comms_begin_pos = pos;
                    lazpos = 1;
                } else if (str.substr(pos, 2) == "*/") {
                    throw OldParseError(lines, "unexpected comment closing: \"*/\"", lno, pos, 2);
                } else if (str.substr(pos, 2) == "//") {
                    check_token_tail = true;
                    break;
                } else if (spaces.find(str[pos]) != std::string::npos) {
                    check_token_tail = true;
                } else {
                    if (token_head < 0) token_head = pos;
                }
            }
            if (check_token_tail && token_head >= 0) {
                tokens.emplace_back(lno, token_head, pos - token_head);
                // tokens.emplace_back(str.substr(token_head, pos - token_head));
                token_head = -1;
            }
            pos += lazpos;
            lazpos = 0;
        }
        if (token_head >= 0) {
            // tokens.emplace_back(str.substr(token_head));
            tokens.emplace_back(lno, token_head, m - token_head);
            token_head = -1;
        }
        // std::cerr << "line " << lno << ": \"" << str << "\" -> [";
        // for (auto&& tok : tokens) std::cerr << tok.string(lines) << ", ";
        // std::cerr << "]" << std::endl;
        // std::cerr << "flag: {comm: " << comm << "}" << std::endl;

        if (tokens.size() == 0) continue;

        if (tokens.size() == 1) {
            std::string t = tokens[0].string(lines);
            if (t == "END") {
                if (def_begin >= 0) {
                    throw OldParseErrorWithNote(
                        OldParseError(lines, "expected \"edef2\" before reaching \"END\"", tokens[0]),
                        OldParseError(lines, "to match this \"def2\"", last_def2));
                }
                eof = true;
                break;
            } else if (t == "def2") {
                if (def_begin < 0) {
                    def_begin = lno;
                    last_def2 = tokens[0];
                } else throw OldParseErrorWithNote(
                    OldParseError(lines, "expected \"edef2\" at end of definition", tokens[0]),
                    OldParseError(lines, "to match this \"def2\"", last_def2));
            } else if (t == "edef2") {
                if (def_begin < 0) throw OldParseError(lines, "expected \"def2\" before \"edef2\"", tokens[0]);
                else {
                    std::cerr << "# of lines in def: " << tokenss.size() << std::endl;
                    tokenss.emplace_back(tokens);
                    env.emplace_back(parse_def(lines, tokenss));
                    tokenss.clear();
                    def_begin = -1;
                }
            }
        }
        if (def_begin >= 0) tokenss.emplace_back(tokens);
    }
    if (comm) {
        throw OldParseError(lines, "unterminated comment", last_comms_begin_lno, last_comms_begin_pos, 2);
    }
    if (!eof) {
        throw OldParseError(lines, "reached end of file while parsing (did you forget \"END\"?)", lines.size() - 1, 1);
    }

    return std::make_shared<Environment>(env);
}
