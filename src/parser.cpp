#include "parser.hpp"

#include <iostream>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

#include "common.hpp"

std::string to_string(const TokenType& t) {
    switch (t) {
        case TokenType::Unclassified: return "TokenType::Unclassified";
        case TokenType::NewLine: return "TokenType::NewLine";
        case TokenType::Number: return "TokenType::Number";
        case TokenType::Character: return "TokenType::Character";
        case TokenType::String: return "TokenType::String";
        case TokenType::SquareBracketLeft: return "TokenType::SquareBracketLeft";
        case TokenType::SquareBracketRight: return "TokenType::SquareBracketRight";
        case TokenType::Comma: return "TokenType::Comma";
        case TokenType::Colon: return "TokenType::Colon";
        case TokenType::Semicolon: return "TokenType::Semicolon";
        case TokenType::Backslash: return "TokenType::Backslash";
        case TokenType::Period: return "TokenType::Period";
        case TokenType::ParenLeft: return "TokenType::ParenLeft";
        case TokenType::ParenRight: return "TokenType::ParenRight";
        case TokenType::CurlyBracketLeft: return "TokenType::CurlyBracketLeft";
        case TokenType::CurlyBracketRight: return "TokenType::CurlyBracketRight";
        case TokenType::DollarSign: return "TokenType::DollarSign";
        case TokenType::QuestionMark: return "TokenType::QuestionMark";
        case TokenType::Percent: return "TokenType::Percent";
        case TokenType::Asterisk: return "TokenType::Asterisk";
        case TokenType::AtSign: return "TokenType::AtSign";
        case TokenType::Hash: return "TokenType::Hash";
        case TokenType::Underscore: return "TokenType::Underscore";
        case TokenType::Hyphen: return "TokenType::Hyphen";
        case TokenType::Verticalbar: return "TokenType::Verticalbar";
        case TokenType::Leftarrow: return "TokenType::Leftarrow";
        case TokenType::Rightarrow: return "TokenType::Rightarrow";
        case TokenType::Leftrightarrow: return "TokenType::Leftrightarrow";
        case TokenType::Leftdoublearrow: return "TokenType::Leftdoublearrow";
        case TokenType::Rightdoublearrow: return "TokenType::Rightdoublearrow";
        case TokenType::Leftrightdoublearrow: return "TokenType::Leftrightdoublearrow";
        case TokenType::DefinedBy: return "TokenType::DefinedBy";
        case TokenType::DefBegin: return "TokenType::DefBegin";
        case TokenType::DefEnd: return "TokenType::DefEnd";
        case TokenType::EndOfFile: return "TokenType::EndOfFile";
        case TokenType::Unknown: return "TokenType::Unknown";
        default: {
            check_true_or_exit(
                false,
                "unknown TokenType value (" << (int)t << ")",
                __FILE__, __LINE__, __func__);
        }
    }
}

TokenType sym2tokentype(char ch) {
    switch (ch) {
        case '(': return TokenType::ParenLeft;
        case ')': return TokenType::ParenRight;
        case '[': return TokenType::SquareBracketLeft;
        case ']': return TokenType::SquareBracketRight;
        case '{': return TokenType::CurlyBracketLeft;
        case '}': return TokenType::CurlyBracketRight;
        case '$': return TokenType::DollarSign;
        case '?': return TokenType::QuestionMark;
        case '@': return TokenType::AtSign;
        case '*': return TokenType::Asterisk;
        case ':': return TokenType::Colon;
        case '.': return TokenType::Period;
        case ',': return TokenType::Comma;
        case '\\': return TokenType::Backslash;
        case ';': return TokenType::Semicolon;
        case '#': return TokenType::Hash;
        case '%': return TokenType::Percent;
        case '|': return TokenType::Verticalbar;
        case '_': return TokenType::Underscore;
        case '-': return TokenType::Hyphen;
        default: return TokenType::Unknown;
    }
}

std::ostream& operator<<(std::ostream& os, const TokenType& t) {
    os << to_string(t);
    return os;
}

std::string pos_info_str(const Token& t1, const Token& t2) {
    if (t1.lno() > t2.lno()) return pos_info_str(t2, t1);
    std::string res(t1.filename());
    if (res.size() > 0) res += ":";
    if (t1.lno() == t2.lno()) {
        res += std::to_string(t1.lno() + 1);
        size_t from, to;
        from = std::min(t1.pos(), t2.pos());
        to = std::max(t1.pos() + t1.len(), t2.pos() + t2.len());
        res += ":" + std::to_string(from + 1);
        if (to - from > 1) res += "-" + std::to_string(std::min(to, t1.filedata()[t1.lno()].size()) + 1);
    } else {
        res += std::to_string(t1.lno() + 1);
        res += ":" + std::to_string(t1.pos() + 1);
        res += "-" + std::to_string(t2.lno() + 1);
        res += ":" + std::to_string(t2.pos() + t2.len());
    }
    return res;
}
std::string pos_info_str(const Token& t) { return pos_info_str(t, t); }

std::string to_string(const Token& t) {
    return "Token[" + pos_info_str(t) + ": \"" + t.string() + "\"; " + to_string(t.type()) + "]";
}

std::ostream& operator<<(std::ostream& os, const Token& t) {
    return os << to_string(t);
}

std::vector<Token> tokenize(const FileData& lines) {
    std::vector<Token> tokens;
    // const std::string sym_const = "_-";
    bool comment = false;
    for (size_t lno = 0; lno < lines.size(); ++lno) {
        for (size_t pos = 0; pos < lines[lno].size();) {
            char ch = lines[lno][pos];
            if (comment) {
                if (ch == '*' && pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '/') {
                    comment = false;
                    pos += 2;
                    continue;
                }
                ++pos;
                continue;
            }
            if (ch == '/') {
                if (pos + 1 < lines[lno].size()) {
                    if (lines[lno][pos + 1] == '/') break;
                    if (lines[lno][pos + 1] == '*') {
                        comment = true;
                        ++pos;
                        continue;
                    }
                }
            }
            if (ch == ':') {
                if (pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '=') {
                    tokens.emplace_back(lines, lno, pos, 2, TokenType::DefinedBy);
                    pos += 2;
                    continue;
                }
            }
            if (ch == '<') {
                if (pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '-') {
                    if (pos + 2 < lines[lno].size() && lines[lno][pos + 2] == '>') {
                        tokens.emplace_back(lines, lno, pos, 3, TokenType::Leftrightarrow);
                        pos += 3;
                        continue;
                    }
                    tokens.emplace_back(lines, lno, pos, 2, TokenType::Leftarrow);
                    pos += 2;
                    continue;
                }
                if (pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '=') {
                    if (pos + 2 < lines[lno].size() && lines[lno][pos + 2] == '>') {
                        tokens.emplace_back(lines, lno, pos, 3, TokenType::Leftrightdoublearrow);
                        pos += 3;
                        continue;
                    }
                    tokens.emplace_back(lines, lno, pos, 2, TokenType::Leftdoublearrow);
                    pos += 2;
                    continue;
                }
            }
            if (ch == '-') {
                if (pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '>') {
                    tokens.emplace_back(lines, lno, pos, 2, TokenType::Rightarrow);
                    pos += 2;
                    continue;
                }
            }
            if (ch == '=') {
                if (pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '>') {
                    tokens.emplace_back(lines, lno, pos, 2, TokenType::Rightdoublearrow);
                    pos += 2;
                    continue;
                }
            }
            if (isalpha(ch)) {
                // variable, name, def2, edef2, END
                size_t eon = pos;
                while (eon < lines[lno].size() && (isalnum(lines[lno][eon]) || lines[lno][eon] == '_')) ++eon;
                std::string tokstr = lines[lno].substr(pos, eon - pos);
                TokenType t;
                if (tokstr == "END") t = TokenType::EndOfFile;
                else if (tokstr == "edef2") t = TokenType::DefEnd;
                else if (tokstr == "def2") t = TokenType::DefBegin;
                else if (pos + 1 < eon) t = TokenType::String;
                else t = TokenType::Character;
                tokens.emplace_back(lines, lno, pos, eon - pos, t);
                pos = eon;
                continue;
            }
            if (isdigit(ch)) {
                size_t eod = pos;
                while (eod < lines[lno].size() && isdigit(lines[lno][eod])) ++eod;
                tokens.emplace_back(lines, lno, pos, eod - pos, TokenType::Number);
                pos = eod;
                continue;
            }
            if (ch == ' ' || ch == '\t') {
                ++pos;
                continue;
            }
            tokens.emplace_back(lines, lno, pos, 1, sym2tokentype(ch));
            if (tokens.back().type() == TokenType::Unknown) throw TokenizeError("unknown token found", tokens.back());

            ++pos;
        }
        tokens.emplace_back(lines, lno, TokenType::NewLine);
    }
    return tokens;
}

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx, bool no_chainer) {
    auto incr_idx = [&](std::string expect, std::string parsing, int hdr, int ofs = -1) {
        if (idx + 1 >= tokens.size()) throw ExprError(
            "expected " + expect + ", reached end of tokens", tokens[idx],
            "during parsing " + parsing, tokens[hdr], tokens[idx + ofs]);
        ++idx;
    };

    std::shared_ptr<Term> res_ptr = nullptr;
    int res_from = -1, res_to = -1;

    switch (tokens[idx].type()) {
        case TokenType::Character: {
            res_from = idx;
            res_ptr = variable(tokens[idx].string()[0]);
            break;
        }
        case TokenType::Asterisk: {
            res_from = idx;
            res_ptr = star;
            break;
        }
        case TokenType::AtSign: {
            res_from = idx;
            res_ptr = sq;
            break;
        }
        case TokenType::ParenLeft: {
            size_t parleft = idx;
            auto content = parse_lambda(tokens, ++idx, false);
            if (idx + 1 >= tokens.size() || tokens[idx + 1].type() != TokenType::ParenRight) throw ExprError(
                "parenthesis not closed", tokens[idx],
                "opens here", tokens[parleft]);
            ++idx;
            res_from = parleft;
            res_to = idx;
            res_ptr = content->term();
            break;
        }
        case TokenType::Percent: {
            size_t appl_hdr = idx;
            std::shared_ptr<ParseLambdaToken> expr1, expr2;

            // if (idx + 1 >= tokens.size()) throw ExprError(
            //     "expected an expr, reached end of tokens", tokens[idx],
            //     "during parsing an application", tokens[appl_hdr], tokens[idx]);
            incr_idx("an expr", "an application", appl_hdr, 0);
            try {
                expr1 = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 1st expr of application", tokens[idx],
                    "application starts here", tokens[appl_hdr]);
            }
            incr_idx("an expr", "an application", appl_hdr);
            try {
                expr2 = parse_lambda(tokens, idx, true);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 2nd expr of application", tokens[idx],
                    "application starts here", tokens[appl_hdr]);
            }
            res_from = appl_hdr;
            res_to = idx;
            res_ptr = appl(expr1->term(), expr2->term());
            break;
        }
        case TokenType::DollarSign:
        case TokenType::QuestionMark: {
            size_t abst_hdr = idx;
            bool is_lambda = (tokens[idx].type() == TokenType::DollarSign ? true : false);
            std::shared_ptr<ParseLambdaToken> var, expr1, expr2;
            incr_idx("a variable", "an abstraction", abst_hdr, 0);
            try {
                var = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing bind variable of abstraction", tokens[idx],
                    "abstraction starts here", tokens[abst_hdr]);
            }
            if (var->term()->etype() != EpsilonType::Variable) throw ExprError("expected a variable", tokens[idx]);
            incr_idx("a colon", "an abstraction", abst_hdr);
            if (tokens[idx].type() != TokenType::Colon) throw ExprError(
                "expected colon, got an invalid token", tokens[idx],
                "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            if (idx + 1 >= tokens.size()) throw ExprError("reached end of tokens during parsing expr", tokens[idx]);
            ++idx;
            try {
                expr1 = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 1st expr of an abstraction", tokens[idx],
                    "abstraction starts here", tokens[abst_hdr]);
            }
            incr_idx("a period", "an abstraction", abst_hdr);
            if (tokens[idx].type() != TokenType::Period) throw ExprError(
                "expected period, got an invalid token", tokens[idx],
                "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            if (idx + 1 >= tokens.size()) throw ExprError("reached end of tokens during parsing expr", tokens[idx]);
            ++idx;
            try {
                expr2 = parse_lambda(tokens, idx, true);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 2nd expr of abstraction", tokens[idx],
                    "abstraction starts here", tokens[abst_hdr]);
            }
            res_from = abst_hdr;
            res_to = idx;
            if (is_lambda) res_ptr = lambda(var->term(), expr1->term(), expr2->term());
            else res_ptr = pi(var->term(), expr1->term(), expr2->term());
            break;
        }
        case TokenType::String: {
            size_t const_hdr = idx;
            std::string cname = tokens[const_hdr].string();
            bool reach_left = false;
            while (!reach_left) {
                incr_idx("an open square bracket", "a constant", const_hdr, 0);
                switch (tokens[idx].type()) {
                    case TokenType::SquareBracketLeft:
                        reach_left = true;
                        break;
                    case TokenType::Number:
                    case TokenType::Underscore:
                    case TokenType::String:
                    case TokenType::Period:
                    case TokenType::Character:
                        cname += tokens[idx].string();
                        break;
                    default:
                        throw ExprError(
                            "expected an open square bracket, got an invalid token", tokens[idx],
                            "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
                }
            }

            incr_idx("an expr or a closing square bracket", "a constant", const_hdr);
            std::vector<std::shared_ptr<Term>> parameters;
            if (tokens[idx].type() != TokenType::SquareBracketRight) {
                while (true) {
                    try {
                        parameters.emplace_back(parse_lambda(tokens, idx)->term());
                    } catch (ExprError& e) {
                        e.puterror();
                        throw ExprError(
                            "above error raised during parsing #" + std::to_string(parameters.size() + 1) + " expr of constant", tokens[idx],
                            "constant starts here", tokens[const_hdr]);
                    }
                    incr_idx("a comma or a closing square bracket", "a constant", const_hdr);
                    if (tokens[idx].type() == TokenType::Comma) {
                        incr_idx("an expr", "a constant", const_hdr);
                    } else if (tokens[idx].type() == TokenType::SquareBracketRight) break;
                    else throw ExprError(
                        "expected an expr, got an invalid token", tokens[idx],
                        "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
                }
            }
            res_from = const_hdr;
            res_to = idx;
            res_ptr = constant(cname, parameters);
            break;
        }
        default:
            throw ExprError("invalid token", tokens[idx]);
    }

    if (idx + 1 < tokens.size() && !no_chainer) {
        std::string chainer_name;
        switch (tokens[idx + 1].type()) {
            case TokenType::Rightarrow:
                chainer_name = "right arrow";
                break;
            case TokenType::Rightdoublearrow:
                chainer_name = "right double arrow";
                break;
            case TokenType::Leftrightdoublearrow:
                chainer_name = "left right double arrow";
                break;
            default:
                break;
        }
        if (chainer_name.size() > 0) {
            size_t chainer_pos = ++idx;
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected an expr after " + chainer_name + ", reached end of tokens", tokens[chainer_pos],
                "expr before " + chainer_name, tokens[res_from], tokens[res_to]);
            ++idx;
            std::shared_ptr<ParseLambdaToken> plt;
            try {
                plt = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing expr", tokens[idx],
                    "after this " + chainer_name, tokens[chainer_pos]);
            }
            std::shared_ptr<Term> expr = plt->term(), combined;
            switch (tokens[chainer_pos].type()) {
                case TokenType::Rightarrow:
                    combined = pi(get_fresh_var(res_ptr, expr), res_ptr, expr);
                    break;
                case TokenType::Rightdoublearrow:
                    combined = constant("implies", {res_ptr, expr});
                    break;
                case TokenType::Leftrightdoublearrow:
                    combined = constant("equiv", {res_ptr, expr});
                    break;
                default:
                    check_true_or_exit(
                        false,
                        "unreachable point (please report this bug)",
                        __FILE__, __LINE__, __func__);
            }
            return std::make_shared<ParseLambdaToken>(tokens[res_from], tokens[idx], combined);
        }
    }
    return res_to < 0
               ? std::make_shared<ParseLambdaToken>(tokens[res_from], res_ptr)
               : std::make_shared<ParseLambdaToken>(tokens[res_from], tokens[res_to], res_ptr);
    throw ParseError("reached end of parse_lambda()", tokens[idx]);
}

std::shared_ptr<Term> parse_lambda(const std::string& str) {
    size_t idx;
    return parse_lambda(tokenize(FileData({str}, "")), idx = 0)->term();
}

Environment parse_defs(const std::vector<Token>& tokens) {
    Environment env;
    // state variables
    bool eof = false;
    int in_def = -1;     // def2 - edef2
    int cont_line = -1;  // backslash line continuation
    // definition component
    bool read_def_num = false;
    bool read_def_context = false;
    bool read_def_context_var = false;
    bool read_def_context_col = false;
    bool read_def_name = false;
    bool read_def_term = false;
    bool read_def_term_defby = false;
    bool read_def_type = false;
    bool read_def_type_col = false;
    bool read_def_end = false;
    // definition: size of context
    int def_num = 0;
    // read lambda expression
    bool read_lambda = false;

    // read flag
    bool vbar_head_of_line = true;
    bool read_flag_context = false;
    bool read_flag_context_type = false;
    bool read_flag_context_end = false;
    bool read_flag_cname = false;
    bool read_flag_defby = false;
    bool read_flag_stmt_term = false;
    bool read_flag_stmt_col = false;
    bool read_flag_stmt_type = false;

    std::shared_ptr<Context> context;  // = std::make_shared<Context>();
    std::string cname;
    std::shared_ptr<Term> term, type;

    std::queue<std::shared_ptr<Variable>> temp_vars;

    std::vector<std::shared_ptr<Context>> flag_context;
    size_t flag_line_num = 0;

    auto def_str = [&]() {
        std::string res("");
        res += (read_def_num ? 'Z' : '-');
        res += (read_def_context ? 'C' : '-');
        res += (read_def_context_var ? 'v' : '-');
        res += (read_def_context_col ? 'c' : '-');
        res += (read_def_name ? 'N' : '-');
        res += (read_def_term ? 'T' : '-');
        res += (read_def_term_defby ? 'd' : '-');
        res += (read_def_type ? 'Y' : '-');
        res += (read_def_type_col ? 'c' : '-');
        res += (read_def_end ? 'E' : '-');
        return res;
    };
    unused(def_str);

    auto flg_str = [&]() {
        std::string res("");
        res += "[" + std::to_string(flag_line_num) + "]";
        res += (vbar_head_of_line ? 'V' : '-');
        res += (read_flag_context ? 'C' : '-');
        res += (read_flag_context_type ? 't' : '-');
        res += (read_flag_context_end ? 'E' : '-');
        res += "/";
        res += (read_flag_cname ? 'N' : '-');
        res += (read_flag_defby ? 'D' : '-');
        res += (read_flag_stmt_term ? 'T' : '-');
        res += (read_flag_stmt_col ? 'c' : '-');
        res += (read_flag_stmt_type ? 't' : '-');
        return res;
    };
    unused(flg_str);

    for (size_t idx = 0; !eof && idx < tokens.size(); ++idx) {
        if (DEBUG_CERR) std::cerr << "[debug; parse loop] flag[def]: " << def_str() << std::endl;
        if (DEBUG_CERR) {
            std::cerr << "[debug; parse loop] flag[flg]: " << flg_str() << ", token: \"" << (tokens[idx].type() == TokenType::NewLine ? "<NL>" : tokens[idx].string()) << "\"\t";
            std::cerr << "flag context dump (size: " << flag_context.size() << "): ";
            for (auto&& c : flag_context) {
                std::cerr << "[";
                if (c) std::cerr << c;
                else std::cerr << "NUL";
                std::cerr << "], ";
            }
            std::cerr << std::endl;
        }
        auto& t = tokens[idx];
        if (read_lambda) {
            std::shared_ptr<ParseLambdaToken> expr;
            ParseError invalid_err("expected expr, got an invalid token", t);
            switch (t.type()) {
                case TokenType::NewLine:
                case TokenType::Semicolon:
                    continue;
                case TokenType::Asterisk:
                case TokenType::AtSign:
                case TokenType::ParenLeft:
                case TokenType::DollarSign:
                case TokenType::QuestionMark:
                case TokenType::Percent:
                case TokenType::String:
                case TokenType::Character: {
                    size_t idx0 = idx;
                    try {
                        expr = parse_lambda(tokens, idx);
                    } catch (const ExprError& e) {
                        ParseError err("failed to parse a lambda expression", tokens[idx0]);
                        err.bind(e);
                        throw err;
                    }
                    break;
                }
                case TokenType::DefinedBy:
                    if (read_def_term) {
                        if (!read_def_term_defby) {
                            read_def_term_defby = true;
                            continue;
                        }
                    }
                    throw invalid_err;
                case TokenType::Hash:
                    term = nullptr;
                    if (read_def_term) {
                        read_def_term = false;
                        read_def_term_defby = false;
                        read_def_type = true;
                        continue;
                    }
                    if (read_flag_stmt_term) {
                        read_flag_stmt_term = false;
                        read_flag_stmt_col = true;
                        read_lambda = false;
                        continue;
                    }
                    throw invalid_err;
                case TokenType::Comma:
                    if (read_def_context && !read_def_context_var) {
                        read_def_context_var = true;
                        continue;
                    }
                    throw invalid_err;
                case TokenType::Colon:
                    if (read_def_context && !read_def_context_var) {
                        if (!read_def_context_col) {
                            read_def_context_col = true;
                            continue;
                        }
                    }
                    if (read_def_type) {
                        if (!read_def_type_col) {
                            read_def_type_col = true;
                            continue;
                        }
                    }
                    throw invalid_err;
                case TokenType::Backslash:
                    if (idx + 1 < tokens.size() && tokens[idx + 1].type() == TokenType::NewLine) continue;
                    throw invalid_err;
                default:
                    throw ParseError("expected expr, got an invalid token", t);
            }
            // got an expr
            read_lambda = false;
            if (read_def_context) {
                if (read_def_context_var) {
                    if (expr->term()->etype() != EpsilonType::Variable) throw ParseError(
                        "expected a variable, got " + to_string(expr->term()->etype()),
                        expr->begin(),
                        expr->end());
                    temp_vars.push(variable(expr->term()));
                    read_def_context_var = false;
                    read_def_context_col = false;
                    read_lambda = true;
                } else {
                    while (!temp_vars.empty()) {
                        auto q = temp_vars.front();
                        temp_vars.pop();
                        context->emplace_back(q, expr->term());
                        if (--def_num < 0) throw ParseError("too many statements", t);
                    }
                    read_def_context_col = false;
                    if (def_num == 0) {
                        read_def_context = false;
                        read_def_name = true;
                    } else {
                        read_def_context_var = true;
                        read_lambda = true;
                    }
                }
            } else if (read_def_term) {
                term = expr->term();
                read_def_term = false;
                read_def_term_defby = false;
                read_def_type = true;
                read_lambda = true;
            } else if (read_def_type) {
                type = expr->term();
                read_def_type = false;
                read_def_type_col = false;
                read_def_end = true;
            } else if (read_flag_context) {
                if (read_flag_context_type) {
                    while (!temp_vars.empty()) {
                        auto q = temp_vars.front();
                        temp_vars.pop();
                        flag_context[flag_line_num]->emplace_back(q, expr->term());
                    }
                    read_flag_context_type = false;
                    read_lambda = false;
                } else {
                    if (expr->term()->etype() != EpsilonType::Variable) throw ParseError(
                        "expected a variable, got " + to_string(expr->term()->etype()),
                        expr->begin(),
                        expr->end());
                    temp_vars.push(variable(expr->term()));
                    read_lambda = false;
                }
            } else if (read_flag_stmt_term) {
                term = expr->term();
                read_flag_stmt_term = false;
                read_flag_stmt_col = true;
                read_lambda = false;
            } else if (read_flag_stmt_type) {
                type = expr->term();
                read_flag_stmt_type = false;
                context = std::make_shared<Context>();
                for (size_t i = 0; i < flag_line_num; ++i) *context += *flag_context[i];
                if (term) env.push_back(std::make_shared<Definition>(context, cname, term, type));
                else env.push_back(std::make_shared<Definition>(context, cname, type));
            }
            continue;
        }
        if (cont_line >= 0 && t.type() != TokenType::NewLine) throw ParseError(
            "expected end of line, got an invalid token", t,
            "after this backslash", tokens[cont_line]);
        switch (t.type()) {
            case TokenType::Backslash: {
                if (cont_line < 0) cont_line = idx;
                break;
            }
            case TokenType::Number: {
                if (!read_def_num) throw ParseError("invalid numeral", t);
                int i = std::stoi(t.string());
                if (i < 0) throw ParseError("the number of elements in a context should be given as nonnegative", t);
                def_num = i;
                read_def_num = false;

                context = std::make_shared<Context>();
                if (def_num > 0) {
                    read_def_context = true;
                    read_def_context_var = true;
                    read_lambda = true;
                } else {
                    read_def_name = true;
                }

                break;
            }
            case TokenType::DefBegin: {
                if (in_def >= 0) throw ParseError(
                    "you need to close definition with \"edef2\" before beginning new one", t,
                    "unclosed definition begins here", tokens[in_def]);
                in_def = idx;
                // definition parse state initialization
                read_def_context = read_def_name = read_def_term = false;
                read_def_type = read_def_end = false;
                read_def_context_var = read_lambda = false;
                read_def_num = true;
                break;
            }
            case TokenType::DefEnd: {
                if (!read_def_end || in_def < 0) throw ParseError("invalid token", t);
                // add definition to env
                read_def_end = false;
                if (term) {
                    env.push_back(std::make_shared<Definition>(
                        context,
                        cname,
                        term, type));
                } else {
                    env.push_back(std::make_shared<Definition>(
                        context,
                        cname,
                        type));
                }
                // context->clear();
                in_def = -1;
                break;
            }
            case TokenType::Character:
            case TokenType::String: {
                if (read_def_name || read_flag_cname) {
                    cname = t.string();
                    while (tokens[idx + 1].type() != TokenType::NewLine && tokens[idx + 1].type() != TokenType::DefinedBy) {
                        cname += tokens[++idx].string();
                    }
                }
                if (read_def_name) {
                    read_def_name = false;
                    read_def_term = true;
                    read_lambda = true;
                    break;
                } else if (read_flag_cname) {
                    read_flag_cname = false;
                    vbar_head_of_line = false;
                    read_flag_defby = true;
                    break;
                }
                throw ParseError("invalid token", t);
            }
            case TokenType::EndOfFile: {
                if (in_def >= 0) throw ParseError(
                    "reached end of file in definition", t,
                    "definition starts here", tokens[in_def]);
                eof = true;
                continue;
            }
            case TokenType::SquareBracketLeft: {
                vbar_head_of_line = false;
                read_flag_context = true;
                read_flag_context_type = false;
                read_lambda = true;
                if (flag_line_num < flag_context.size()) flag_context[flag_line_num] = std::make_shared<Context>();
                else flag_context.push_back(std::make_shared<Context>());
                continue;
            }
            case TokenType::SquareBracketRight: {
                if (read_flag_context) {
                    read_flag_context = false;
                    read_flag_context_end = true;
                    continue;
                }
                throw ParseError("invalid token", t);
            }
            case TokenType::Comma: {
                if (read_flag_context && !read_lambda) {
                    read_lambda = true;
                    continue;
                }
                throw ParseError("invalid token", t);
            }
            case TokenType::Colon: {
                if (read_flag_context) {
                    read_flag_context_type = true;
                    read_lambda = true;
                    continue;
                } else if (read_flag_stmt_col) {
                    read_flag_stmt_col = false;
                    read_flag_stmt_type = true;
                    read_lambda = true;
                    continue;
                }
                throw ParseError("invalid token", t);
            }
            case TokenType::Verticalbar: {
                if (!vbar_head_of_line) throw ParseError("vertical bar '|' should be placed at the head of line", t);
                ++flag_line_num;
                if (flag_context.size() < flag_line_num || !flag_context[flag_line_num - 1]) {
                    std::cerr << "flag context dump (size: " << flag_context.size() << "): ";
                    for (auto&& c : flag_context) {
                        std::cerr << "[";
                        if (c) std::cerr << c;
                        else std::cerr << "NUL";
                        std::cerr << "], ";
                    }
                    std::cerr << std::endl;
                    throw ParseError("context of " + std::to_string(flag_line_num) + "-th flag is undefined", t);
                }
                continue;
            }
            case TokenType::DefinedBy: {
                if (read_flag_defby) {
                    read_flag_defby = false;
                    read_flag_stmt_term = true;
                    read_lambda = true;
                    continue;
                }
                throw ParseError("invalid token", t);
            }
            case TokenType::NewLine: {
                if (!read_flag_context_end && flag_line_num < flag_context.size()) flag_context.resize(flag_line_num);
                flag_line_num = 0;
                vbar_head_of_line = true;
                read_flag_cname = true;
                read_flag_context_end = false;
                continue;
            }
            default:
                throw ParseError("not implemented in parse_defs() (token = " + to_string(t) + ")", t);
        }
    }
    return env;
}
