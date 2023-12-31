#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "common.hpp"
#include "parser.hpp"


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
        case TokenType::DefinedBy: return "TokenType::DefinedBy";
        case TokenType::DefBegin: return "TokenType::DefBegin";
        case TokenType::DefEnd: return "TokenType::DefEnd";
        case TokenType::EndOfFile: return "TokenType::EndOfFile";
        case TokenType::Unknown: return "TokenType::Unknown";
        default: {
            std::cerr << "unknown TokenType value (" << (int)t << ")" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

TokenType sym2tokentype(char ch) {
    switch (ch) {
        case '(': return TokenType::ParenLeft;
        case ')': return TokenType::ParenRight;
        case '[': return TokenType::SquareBracketLeft;
        case ']': return TokenType::SquareBracketRight;
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
        default: return TokenType::Unknown;
    }
}

std::ostream& operator<<(std::ostream& os, const TokenType& t) {
    os << to_string(t);
    return os;
}

std::string to_string(const Token& t) {
    return "Token[" + t.pos_info_str() + ": \"" + t.string() + "\"; " + to_string(t.type()) + "]";
}

std::ostream& operator<<(std::ostream& os, const Token& t) {
    return os << to_string(t);
}

std::vector<Token> tokenize(const FileData& lines) {
    std::vector<Token> tokens;
    const std::string sym_const = "_-";
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
            if (isalpha(ch)) {
                // variable, name, def2, edef2, END
                size_t eon = pos;
                while (eon < lines[lno].size() && (isalnum(lines[lno][eon]) || sym_const.find(lines[lno][eon]) != std::string::npos)) ++eon;
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

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx) {
    switch (tokens[idx].type()) {
        case TokenType::Character: {
            return std::make_shared<ParseLambdaToken>(
                tokens[idx],
                // ParseLambdaState::Term,
                std::make_shared<Variable>(tokens[idx].string()[0]));
        }
        case TokenType::Asterisk: {
            return std::make_shared<ParseLambdaToken>(
                tokens[idx],
                // ParseLambdaState::Term,
                std::make_shared<Star>());
        }
        case TokenType::AtSign: {
            return std::make_shared<ParseLambdaToken>(
                tokens[idx],
                // ParseLambdaState::Term,
                std::make_shared<Square>());
        }
        case TokenType::ParenLeft: {
            size_t parleft = idx;
            auto content = parse_lambda(tokens, ++idx);
            if (idx + 1 >= tokens.size() || tokens[idx + 1].type() != TokenType::ParenRight) throw ExprError(
                "parenthesis not closed", tokens[idx],
                "opens here", tokens[parleft]);
            ++idx;
            return std::make_shared<ParseLambdaToken>(
                tokens[parleft],
                tokens[idx],
                content->term());
        }
        case TokenType::Percent: {
            size_t appl_hdr = idx;
            std::shared_ptr<ParseLambdaToken> expr1, expr2;
            // if (idx + 1 < tokens.size()) throw ExprError("reached end of tokens during parsing expr", tokens[idx]);
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected an expr, reached end of tokens", tokens[idx],
                "during parsing an application", tokens[appl_hdr], tokens[idx]);
            ++idx;
            try {
                expr1 = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 1st expr of application", tokens[idx],
                    "application starts here", tokens[appl_hdr]);
            }
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected an expr, reached end of tokens", tokens[idx],
                "during parsing an application", tokens[appl_hdr], tokens[idx - 1]);
            ++idx;
            try {
                expr2 = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 2nd expr of application", tokens[idx],
                    "application starts here", tokens[appl_hdr]);
            }
            return std::make_shared<ParseLambdaToken>(
                tokens[appl_hdr],
                tokens[idx],
                std::make_shared<Application>(expr1->term(), expr2->term()));
        }
        case TokenType::DollarSign:
        case TokenType::QuestionMark: {
            size_t abst_hdr = idx;
            bool is_lambda = (tokens[idx].type() == TokenType::DollarSign ? true : false);
            std::shared_ptr<ParseLambdaToken> var, expr1, expr2;
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected a variable, reached end of tokens", tokens[idx],
                "during parsing an abstraction", tokens[abst_hdr], tokens[idx]);
            ++idx;
            try {
                var = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing bind variable of abstraction", tokens[idx],
                    "abstraction starts here", tokens[abst_hdr]);
            }
            if (var->term()->kind() != Kind::Variable) throw ExprError("expected a variable", tokens[idx]);
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected colon, reached end of tokens", tokens[idx],
                "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            ++idx;
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
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected period, reached end of tokens", tokens[idx],
                "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            ++idx;
            if (tokens[idx].type() != TokenType::Period) throw ExprError(
                "expected period, got an invalid token", tokens[idx],
                "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            if (idx + 1 >= tokens.size()) throw ExprError("reached end of tokens during parsing expr", tokens[idx]);
            ++idx;
            try {
                expr2 = parse_lambda(tokens, idx);
            } catch (ExprError& e) {
                e.puterror();
                throw ExprError(
                    "above error raised during parsing 2nd expr of abstraction", tokens[idx],
                    "abstraction starts here", tokens[abst_hdr]);
            }
            if (is_lambda) return std::make_shared<ParseLambdaToken>(
                tokens[abst_hdr],
                tokens[idx],
                std::make_shared<AbstLambda>(
                    std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(var->term()), expr1->term()),
                    expr2->term()));
            return std::make_shared<ParseLambdaToken>(
                tokens[abst_hdr],
                tokens[idx],
                std::make_shared<AbstPi>(
                    std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(var->term()), expr1->term()),
                    expr2->term()));
        }
        case TokenType::String: {
            size_t const_hdr = idx;
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected a variable, reached end of tokens", tokens[idx],
                "during parsing a constant", tokens[const_hdr], tokens[idx]);
            ++idx;
            if (tokens[idx].type() != TokenType::SquareBracketLeft) throw ExprError(
                "expected open square bracket, got an invalid token", tokens[idx],
                "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
            if (idx + 1 >= tokens.size()) throw ExprError(
                "expected a variable, reached end of tokens", tokens[idx],
                "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
            ++idx;
            std::vector<std::shared_ptr<Term>> types;
            if (tokens[idx].type() != TokenType::SquareBracketRight) {
                while (true) {
                    try {
                        types.emplace_back(parse_lambda(tokens, idx)->term());
                    } catch (ExprError& e) {
                        e.puterror();
                        throw ExprError(
                            "above error raised during parsing #" + std::to_string(types.size() + 1) + " expr of constant", tokens[idx],
                            "constant starts here", tokens[const_hdr]);
                    }
                    if (idx + 1 >= tokens.size()) throw ExprError(
                        "expected a variable, reached end of tokens", tokens[idx],
                        "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
                    ++idx;
                    if (tokens[idx].type() == TokenType::Comma) {
                        if (idx + 1 >= tokens.size()) throw ExprError(
                            "expected an expr, reached end of tokens", tokens[idx],
                            "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
                        ++idx;
                    } else if (tokens[idx].type() == TokenType::SquareBracketRight) break;
                    else throw ExprError(
                        "expected an expr, got an invalid token", tokens[idx],
                        "during parsing a constant", tokens[const_hdr], tokens[idx - 1]);
                }
            }
            return std::make_shared<ParseLambdaToken>(
                tokens[const_hdr],
                tokens[idx],
                std::make_shared<Constant>(
                    tokens[const_hdr].string(),
                    types));
        }
        default:
            throw ExprError("invalid token", tokens[idx]);
    }
    std::cerr << "parse_lambda() end" << std::endl;
    exit(EXIT_FAILURE);
}

Environment parse(const std::vector<Token>& tokens) {
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
    size_t def_num = 0;
    // read lambda expression
    bool read_lambda = false;

    std::vector<std::shared_ptr<Typed<Variable>>> tvars;
    std::vector<std::shared_ptr<Term>> types;
    std::string cname;
    std::shared_ptr<Term> term, type;

    std::shared_ptr<Variable> temp_var;

    auto flag_str = [&]() {
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
    unused(flag_str);

    for (size_t idx = 0; !eof && idx < tokens.size(); ++idx) {
        if (DEBUG_CERR) std::cerr << "[debug; parse loop] flag[def]: " << flag_str() << std::endl;

        auto& t = tokens[idx];
        if (read_lambda) {
            std::shared_ptr<ParseLambdaToken> expr;
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
                    [[fallthrough]];
                case TokenType::Hash:
                    if (read_def_term) {
                        read_def_term = false;
                        read_def_term_defby = false;
                        read_def_type = true;
                        term = nullptr;
                        continue;
                    }
                    [[fallthrough]];
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
                    [[fallthrough]];
                case TokenType::Backslash:
                    if (idx + 1 < tokens.size() && tokens[idx + 1].type() == TokenType::NewLine) continue;
                    [[fallthrough]];
                default:
                    throw ParseError("expected expr, got an invalid token", t);
            }
            // got an expr
            read_lambda = false;
            if (read_def_context) {
                if (read_def_context_var) {
                    if (expr->term()->kind() != Kind::Variable) throw ParseError(
                        "expected a variable, got " + to_string(expr->term()->kind()),
                        expr->begin(),
                        expr->end());
                    temp_var = std::dynamic_pointer_cast<Variable>(expr->term());
                    read_def_context_var = false;
                    read_def_context_col = false;
                    read_lambda = true;
                } else {
                    tvars.emplace_back(std::make_shared<Typed<Variable>>(temp_var, expr->term()));
                    types.emplace_back(expr->term());
                    read_def_context_col = false;
                    if (--def_num == 0) {
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

                if (def_num > 0) {
                    tvars.clear();
                    types.clear();

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
                    env.defs().emplace_back(std::make_shared<Definition>(
                        std::make_shared<Context>(tvars),
                        std::make_shared<Constant>(cname, types),
                        term, type));
                } else {
                    env.defs().emplace_back(std::make_shared<Definition>(
                        std::make_shared<Context>(tvars),
                        std::make_shared<Constant>(cname, types),
                        type));
                }
                tvars.clear();
                types.clear();
                in_def = -1;
                break;
            }
            case TokenType::String: {
                if (!read_def_name) throw ParseError("invalid token", t);
                cname = t.string();
                read_def_name = false;
                read_def_term = true;
                read_lambda = true;
                break;
            }
            case TokenType::EndOfFile: {
                if (in_def >= 0) throw ParseError(
                    "reached end of file in definition", t,
                    "definition starts here", tokens[in_def]);
                eof = true;
                continue;
            }
            case TokenType::NewLine: {
                continue;
            }
            default:
                throw ParseError("not implemented in parse() (token = " + to_string(t) + ")", t);
        }
    }
    return env;
}