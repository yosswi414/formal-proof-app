#include "lambda.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>

struct Location {
    Location() {}
    Location(size_t lno, size_t pos, size_t len) : lno(lno), pos(pos), len(len) {}

    std::string string(const std::vector<std::string>& lines) const { return lines[lno].substr(pos, len); }

    size_t lno, pos, len;
};

using TokenMat = std::vector<std::vector<Location>>;

class ParseError {
  public:
    ParseError(const std::vector<std::string>& lines, const std::string& msg, size_t lno, size_t pos, size_t len = 1) : srcname("unnamed"), msg(msg), lines(lines), loc(lno, pos, len) {}
    ParseError(const std::string& srcname, const std::vector<std::string>& lines, const std::string& msg, size_t lno, size_t pos, size_t len = 1) : srcname(srcname), msg(msg), lines(lines), loc(lno, pos, len) {}

    ParseError(const std::vector<std::string>& lines, const std::string& msg, const Location& loc) : srcname("unnamed"), msg(msg), lines(lines), loc(loc) {}
    ParseError(const std::string& srcname, const std::vector<std::string>& lines, const std::string& msg, const Location& loc) : srcname(srcname), msg(msg), lines(lines), loc(loc) {}

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

class ParseErrorWithNote : public ParseError {
  public:
    ParseErrorWithNote(const ParseError& error, const ParseError& note) : ParseError(error), note(note) {}

    void puterror(std::ostream& os = std::cerr) override {
        static_cast<ParseError>(*this).puterror(os);
        note.puterror(os);
    }
    ParseError note;
};

class LambdaError {
  public:
    LambdaError(const std::string& msg, size_t pos, size_t len): msg(msg), pos(pos), len(len) {}
    std::string msg;
    size_t pos, len;
};

std::shared_ptr<Term> parse_lambda(const std::vector<std::string>& lines, const TokenMat& tokenmat, int& row, int& col, size_t pos = 0) {
    const std::string name_syms("-_.");

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
    if (token(i, j) != "def2") throw ParseError(lines, "(This is a bug. Please report with your input) header is not \"def2\"", tokenmat[i][j]);
    incr(i, j);
    // line 1: # of variables (N)
    size_t num_vars;
    // emptyness check
    try {
        num_vars = std::stoi(token(i, j));
    } catch (const std::invalid_argument& e) {
        throw ParseError(lines, "failed to read a number from this token", tokenmat[i][j]);
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
            throw ParseError(lines, "expected variable (got " + to_string(variable->kind()) + ")", tokenmat[i0][j0]);
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
    if (token(i, j) != "edef2") throw ParseError(lines, "(This is a bug. Please report with your input) footer is not \"edef2\"", tokenmat[i][j]);

    std::shared_ptr<Context> context = std::make_shared<Context>(vars);
    std::shared_ptr<Constant> constant = std::make_shared<Constant>(cname, types);
    return std::make_shared<Definition>(context, constant, proof, prop);
}

// ### special character ###
//      '\' (line continuation) (to be impl'ed),
//      "//" (comment),
//      "/*" (comment start),
//      "*/" (comment end)

// std::string nexttoken(std::string delims, const std::string& text, size_t pos, int end = -1) {
//     int begin = -1;
//     if (end < 0) end = text.size();
//     delims += "\\";
//     for (int i = pos; i < end; ++i) {
//         if (delims.find(text[i]) == delims.size()) {
//             if (begin < 0) begin = i;
//         } else {
//             if (begin >= 0) return text.substr(begin, i - begin);
//         }
//     }
//     if (begin < 0) return "";
//     else return text.substr(begin);
// }

// <line> ::= <token> <line> | <spaces> <line>
// <token> ::= <variable> | <constant_name> | <lambda>
// <variable> ::= [a-zA-Z]
// <constant_name> ::= [a-zA-Z_][a-zA-Z0-9_-\.]*
// <lambda> ::= (symbols and alphabets)

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
                    throw ParseError(lines, "unexpected comment closing: \"*/\"", lno, pos, 2);
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
                    throw ParseErrorWithNote(
                        ParseError(lines, "expected \"edef2\" before reaching \"END\"", tokens[0]),
                        ParseError(lines, "to match this \"def2\"", last_def2)
                    );
                }
                eof = true;
                break;
            } else if (t == "def2") {
                if (def_begin < 0) {
                    def_begin = lno;
                    last_def2 = tokens[0];
                } else throw ParseErrorWithNote(
                    ParseError(lines, "expected \"edef2\" at end of definition", tokens[0]),
                    ParseError(lines, "to match this \"def2\"", last_def2));
            } else if (t == "edef2") {
                if (def_begin < 0) throw ParseError(lines, "expected \"def2\" before \"edef2\"", tokens[0]);
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
        throw ParseError(lines, "unterminated comment", last_comms_begin_lno, last_comms_begin_pos, 2);
    }
    if (!eof) {
        throw ParseError(lines, "reached end of file while parsing (did you forget \"END\"?)", lines.size() - 1, 1);
    }

    return std::make_shared<Environment>(env);
}

int main() {
    std::cout << Kind::Variable << std::endl;
    // Term* ts = new Star();
    std::shared_ptr<Term> ts = std::make_shared<Star>();
    std::shared_ptr<Term> ts2 = std::make_shared<Square>();
    std::cout << ts << " " << ts2 << std::endl;
    std::shared_ptr<Term> ts3 = std::make_shared<Application>(ts, ts2);
    std::cout << ts3 << std::endl;
    std::shared_ptr<Term> ts4 = std::make_shared<Application>(ts3, ts);
    std::cout << ts4 << std::endl;
    std::cout << std::is_pointer<Term*>::value << std::endl;
    std::cout << std::is_pointer<std::shared_ptr<Term>>::value << std::endl;
    std::shared_ptr<Typed<Variable>> tv = std::make_shared<Typed<Variable>>(std::make_shared<Variable>('x'),
                                                                            std::make_shared<Application>(
                                                                                std::make_shared<Star>(),
                                                                                std::make_shared<Star>()));
    std::cout << tv << std::endl;

    std::shared_ptr<Term> tlambda = std::make_shared<AbstLambda>(
        std::make_shared<Typed<Variable>>(std::make_shared<Variable>('x'), std::make_shared<Variable>('A')),
        std::make_shared<Variable>('B'));

    std::cout << "lambda ... " << tlambda << std::endl;

    std::shared_ptr<Term> tconst = std::make_shared<Constant>(
        "hello",
        ts,
        ts3,
        ts2);

    std::cout << "constant ... " << tconst << std::endl;

    std::shared_ptr<Term> tcon0 = std::make_shared<Constant>("world");

    std::cout << "constant ... " << tcon0 << std::endl;

    std::shared_ptr<Term>
        x(std::make_shared<Variable>('x')),
        y(std::make_shared<Variable>('y')),
        z(std::make_shared<Variable>('z')),
        A(std::make_shared<Variable>('A')),
        B(std::make_shared<Variable>('B')),
        C(std::make_shared<Variable>('C')),
        star(std::make_shared<Star>()),
        sq(std::make_shared<Square>());
    // std::cout << x << y << A << B << star << sq << std::endl;

    std::shared_ptr<Typed<Variable>>
        xA(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(x), A)),
        yB(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(y), B)),
        zC(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(z), C));
    // std::cout << xA << std::endl;

    std::shared_ptr<Term>
        PxAB(std::make_shared<AbstPi>(xA, B)),
        PxABz(std::make_shared<Application>(PxAB, z));

    // std::cout << PxABz << std::endl;

    std::shared_ptr<Context>
        conxy(std::make_shared<Context>(xA, yB)),
        conyz(std::make_shared<Context>(yB, zC)),
        conzx(std::make_shared<Context>(zC, xA));

    std::shared_ptr<Constant>
        const1(std::make_shared<Constant>("hoge", A, B)),
        const2(std::make_shared<Constant>("fuga", B, C)),
        const3(std::make_shared<Constant>("piyo", C, A));

    std::shared_ptr<Definition>
        defpr1(std::make_shared<Definition>(conxy, const2, PxAB)),
        defds(std::make_shared<Definition>(conzx, const1, PxABz, PxAB)),
        defpr2(std::make_shared<Definition>(conyz, const3, PxABz));

    std::shared_ptr<Environment>
        defs1(std::make_shared<Environment>(defds, defpr1)),
        defs2(std::make_shared<Environment>(defpr2, defds));

    // std::cout << defs1 << std::endl;

    std::shared_ptr<Judgement>
        judge1(std::make_shared<Judgement>(defs1, conyz, PxAB, star)),
        judge2(std::make_shared<Judgement>(defs2, conxy, star, sq));

    std::shared_ptr<Book>
        book(std::make_shared<Book>(judge1, judge2));

    std::cout << "\n\n";
    std::cout << "defpr1 = " << defpr1 << std::endl;
    std::cout << "defpr2 = " << defpr2 << std::endl;
    std::cout << "defds  = " << defds << std::endl;
    std::cout << "\n";
    std::cout << "env defs1 = " << defs1 << std::endl;
    std::cout << " = \n";
    std::cout << defs1->repr() << std::endl;
    std::cout << "\n";
    std::cout << "judge judge1 = " << judge1 << std::endl;
    std::cout << "judge judge2 = " << judge2->string(false) << std::endl;
    std::cout << "\n";
    std::cout << book << std::endl;

    std::vector<std::string> lines{
        "def2   // (123)",
        "1/*//*/",
        "A",
        "*",
        "const-test  // name",
        "$x:( A ).( * )",
        "@/****//****//**//// //***",
        "/*",
        "*/edef2",
        "",
        "",
        "def2",
        "0",
        "emptydef",
        "*",
        "@",
        "edef2",
        "",
        "def2",
        "edef2",
        "",
        "def2//extended grammar",
        "1",
        "B:*",
        "ext_def := * : @",
        "edef2",
        " \tEND"};
    try {
        std::shared_ptr<Environment> input(parse(lines));
        std::cout << "parse begin" << std::endl;
        std::cout << input << std::endl;
        std::cout << "parse end" << std::endl;
    } catch (ParseError& e) {
        auto ptr = dynamic_cast<ParseErrorWithNote*>(&e);
        if (ptr) ptr->puterror();
        else e.puterror();
        exit(EXIT_FAILURE);
    }
}
