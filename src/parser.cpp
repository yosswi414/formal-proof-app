#include "parser.hpp"

#include <iostream>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

#include "common.hpp"
#include "context.hpp"
#include "definition.hpp"
#include "environment.hpp"

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
        case TokenType::Plus: return "TokenType::Plus";
        case TokenType::Spaces: return "TokenType::Spaces";
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
        case '+': return TokenType::Plus;
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

std::string to_string(const ParseLambdaToken& plt) {
    return "ParseLambdaToken[term = " + plt.term()->string() + ", pos info: " + pos_info_str(plt.begin(), plt.end()) + "]";
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
                size_t eos = pos;
                while (eos < lines[lno].size() && (lines[lno][eos] == ' ' || lines[lno][eos] == '\t')) ++eos;
                tokens.emplace_back(lines, lno, pos, eos - pos, TokenType::Spaces);
                pos = eos;
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

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx, size_t end_of_token, bool exhaust_token, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context, bool no_chainer) {
    // {
    //     std::cerr << "[debug] parse_lambda(): from " << tokens[idx] << " to ";
    //     if (end_of_token == tokens.size()) std::cerr << "end";
    //     else std::cerr << tokens[end_of_token - 1];
    //     std::cerr << std::endl;
    // }
    bool reached_end = false;
    auto skip_spaces = [&]() {
        size_t idx0 = idx;
        while (idx < end_of_token && tokens[idx].type() == TokenType::Spaces) ++idx;
        if (idx == end_of_token) reached_end = true;
        return idx - idx0;
    };
    auto incr_idx = [&](std::string expect, std::string parsing, int hdr, int ofs = -1) {
        if (skip_spaces() > 0) {
            if (reached_end) throw ExprError(
                "expected " + expect + ", reached end of tokens", tokens[idx],
                "during parsing " + parsing, tokens[hdr], tokens[idx + ofs]);
            return;
        }
        if (idx + 1 >= end_of_token) throw ExprError(
            "expected " + expect + ", reached end of tokens", tokens[idx],
            "during parsing " + parsing, tokens[hdr], tokens[idx + ofs]);
        ++idx;
        skip_spaces();
        if (reached_end) throw ExprError(
            "expected " + expect + ", reached end of tokens", tokens[idx],
            "during parsing " + parsing, tokens[hdr], tokens[idx + ofs]);
    };

    std::shared_ptr<Term> res_ptr = nullptr;
    int res_from = -1, res_to = -1;

    skip_spaces();  // guarantees we have something other than spaces
    switch (tokens[idx].type()) {
        case TokenType::Asterisk: {
            res_from = res_to = idx;
            res_ptr = star;
            break;
        }
        case TokenType::AtSign: {
            res_from = res_to = idx;
            res_ptr = sq;
            break;
        }
        case TokenType::ParenLeft: {
            size_t parleft = idx;
            auto content = parse_lambda(tokens, ++idx, end_of_token, flag_context, false);

            incr_idx("a closing parenthesis", "inside a parentheses", parleft);

            if (idx >= end_of_token || tokens[idx].type() != TokenType::ParenRight) {
                throw ExprError(
                    "parenthesis not closed (last expr: " + to_string(*content) + ")", tokens[idx],
                    "last parenthesis opens here", tokens[parleft]);
            }
            res_from = parleft;
            res_to = idx;
            res_ptr = content->term();
            break;
        }
        case TokenType::Percent: {
            size_t appl_hdr = idx;
            std::shared_ptr<ParseLambdaToken> expr1, expr2;
            incr_idx("an expr", "an application", appl_hdr, 0);
            try {
                expr1 = parse_lambda(tokens, idx, end_of_token, flag_context);
            } catch (ExprError& e) {
                ExprError newe(
                    "above error raised during parsing 1st expr of application", tokens[idx],
                    "application starts here", tokens[appl_hdr]);
                newe.chain(e);
                throw newe;
            }
            incr_idx("an expr", "an application", appl_hdr);
            try {
                expr2 = parse_lambda(tokens, idx, end_of_token, exhaust_token, flag_context, true);
            } catch (ExprError& e) {
                ExprError newe(
                    "above error raised during parsing 2nd expr of application (1st expr = " + expr1->term()->string() + ")", tokens[idx],
                    "application starts here", tokens[appl_hdr]);
                newe.chain(e);
                throw newe;
            }
            // std::cerr << "[debug] parsing appl: token at the end: " << tokens[idx] << std::endl;
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
            incr_idx("a bound variable name", "an abstraction", abst_hdr, 0);
            // std::cerr << "[debug] parsing abst: token before parsing bv: " << tokens[idx] << std::endl;
            size_t pos_colon = idx;
            while (pos_colon + 1 < end_of_token && tokens[pos_colon].type() != TokenType::Colon) ++pos_colon;
            if (tokens[pos_colon].type() != TokenType::Colon) throw ExprError(
                "expected colon, not found by here", tokens[pos_colon],
                "during parsing an abstraction", tokens[abst_hdr], tokens[pos_colon - 1]);
            try {
                var = parse_lambda(tokens, idx, pos_colon, true, flag_context);
            } catch (ExprError& e) {
                ExprError newe(
                    "above error raised during parsing bind variable of abstraction", tokens[idx],
                    "abstraction starts here", tokens[abst_hdr]);
                newe.chain(e);
                throw newe;
            }
            // std::cerr << "[debug] parsing abst: token after parsing bv: " << tokens[idx] << std::endl;
            if (var->term()->etype() != EpsilonType::Variable) throw ExprError("expected a variable", tokens[idx]);
            // incr_idx("a colon", "an abstraction", abst_hdr);
            // if (tokens[idx].type() != TokenType::Colon) throw ExprError(
            //     "expected colon, got an invalid token (bound variable: " + to_string(*var) + ")", tokens[idx],
            //     "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            // if (idx + 1 >= end_of_token) throw ExprError("reached end of tokens during parsing expr", tokens[idx]);
            // ++idx;
            idx = pos_colon + 1;
            std::cerr << "[debug] parsing abst: token after colon: " << tokens[idx] << std::endl;
            // incr_idx("a type expr of bound variable", "an abstraction", abst_hdr);
            size_t expr_hdr = idx;
            size_t pos_period = idx;
            // int depth_period = 0;
            // std::map<TokenType, int> depth_brackets;
            
            // int brackets_present = 0;
            int pos_period_last_valid = -1;
            while (!expr2 && tokens[pos_period].type() != TokenType::NewLine) {
                // depth_period = 0;
                std::stack<TokenType> brackets;
                while (pos_period + 1 < end_of_token) {
                    auto ttype = tokens[pos_period].type();
                    bool endwhile = false;
                    switch (ttype) {
                        case TokenType::Period:
                            if (!brackets.empty() && brackets.top() == ttype) brackets.pop();
                            else endwhile = true;
                            break;
                        case TokenType::NewLine:
                            endwhile = true;
                            break;
                        case TokenType::DollarSign:
                        case TokenType::QuestionMark:
                            brackets.push(TokenType::Colon);
                            break;
                        case TokenType::Colon:
                            if (!brackets.empty() && brackets.top() == ttype) {
                                brackets.pop();
                                brackets.push(TokenType::Period);
                            } else endwhile = true;
                            break;
                        case TokenType::SquareBracketLeft:
                            brackets.push(TokenType::SquareBracketRight);
                            break;
                        case TokenType::SquareBracketRight:
                            if (!brackets.empty() && brackets.top() == ttype) brackets.pop();
                            else endwhile = true;
                            break;
                        case TokenType::ParenLeft:
                            brackets.push(TokenType::ParenRight);
                            break;
                        case TokenType::ParenRight:
                            if (!brackets.empty() && brackets.top() == ttype) brackets.pop();
                            else endwhile = true;
                            break;
                        default:
                            break;
                    }
                    if (endwhile) break;

                    ++pos_period;
                }
                if (tokens[pos_period].type() != TokenType::Period) break;
                pos_period_last_valid = pos_period;
                std::cerr << "[debug] parsing abst: " << var->term() << ": first token of 1st expr: " << tokens[idx] << "\tperiod_pos: " << tokens[pos_period] << std::endl;
                try {
                    expr1 = parse_lambda(tokens, idx, pos_period, true, flag_context);
                } catch (ExprError& e) {
                    std::cerr << "[debug] parsing abst: " << var->term() << ":  failed. moving period forward." << std::endl;
                    // e.puterror();
                    ++pos_period;
                    idx = expr_hdr;
                    expr1.reset();
                    continue;
                }
                idx = pos_period + 1;
                std::cerr << "[debug] parsing abst: " << var->term() << ", " << expr1->term() << ": first token of 2nd expr: " << tokens[idx] << "\tperiod_pos: " << tokens[pos_period] << std::endl;
                try {
                    expr2 = parse_lambda(tokens, idx, end_of_token, exhaust_token, flag_context, true);
                    std::cerr << "[debug] parsing abst: " << var->term() << ", " << expr1->term() << ": parsed: " << expr2->term() << std::endl;
                } catch (ExprError& e) {
                    std::cerr << "[debug] parsing abst: " << var->term() << ", " << expr1->term() << ": failed. moving period forward." << std::endl;
                    // e.puterror();
                    ++pos_period;
                    idx = expr_hdr;
                    expr1.reset();
                    expr2.reset();
                    continue;
                }
                std::cerr << "[debug] parsing abst: " << var->term() << ", " << expr1->term() << ", " << expr2->term() << ": done successful." << std::endl;
                break;
            }
            if (!expr2) {
                if (idx != expr_hdr) {
                    std::cerr << "[debug] idx is not rollbacked. this is a bug." << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (pos_period_last_valid < 0) throw ExprError(
                    "failed to parse an abstraction (period not found)", tokens[abst_hdr]);
                throw ExprError(
                    "failed to parse an abstraction", tokens[abst_hdr],
                    "last assumed period symbol (var-expr separation)", tokens[pos_period_last_valid]);
            }
            // incr_idx("a period", "an abstraction", abst_hdr);
            // if (tokens[idx].type() != TokenType::Period) throw ExprError(
            //     "expected period, got an invalid token", tokens[idx],
            //     "during parsing an abstraction", tokens[abst_hdr], tokens[idx - 1]);
            // // if (idx + 1 >= end_of_token) throw ExprError("reached end of tokens during parsing expr", tokens[idx]);
            // // ++idx;
            // incr_idx("an expr", "an abstraction", abst_hdr);
            // try {
            //     expr2 = parse_lambda(tokens, idx, end_of_token, flag_context, true);
            // } catch (ExprError& e) {
            //     e.puterror();
            //     throw ExprError(
            //         "above error raised during parsing 2nd expr of abstraction (bound var: " + to_string(*var) + ", 1st expr: " + to_string(*expr1) + ")", tokens[idx],
            //         "abstraction starts here", tokens[abst_hdr]);
            // }
            res_from = abst_hdr;
            res_to = idx;
            if (is_lambda) res_ptr = lambda(var->term(), expr1->term(), expr2->term());
            else res_ptr = pi(var->term(), expr1->term(), expr2->term());
            break;
        }
        case TokenType::Character:
            // std::cerr << "[debug] got char: " << tokens[idx].string() << std::endl;
            // [[fallthrough]];
        case TokenType::String: {
            size_t identifier_hdr = idx;
            std::string name = tokens[identifier_hdr].string();
            bool reach_left = false;
            bool name_confirmed = false;
            // std::cerr << "[debug] parsing identifier: token at beginning: " << tokens[idx] << std::endl;
            while (!reach_left) {
                if (idx + 1 >= end_of_token) {
                    res_from = identifier_hdr;
                    res_to = idx;
                    res_ptr = variable(name);
                    // std::cerr << "[debug] read variable: " << res_ptr << std::endl;
                    break;
                }
                if (res_ptr) break;
                ++idx;
                // incr_idx("an open square bracket", "a constant", identifier_hdr, 0);
                switch (tokens[idx].type()) {
                    case TokenType::SquareBracketLeft:
                        reach_left = true;
                        name_confirmed = true;
                        break;
                    case TokenType::Period:
                    case TokenType::Number:
                    case TokenType::Underscore:
                    case TokenType::String:
                    case TokenType::Character:
                        if (name_confirmed) {
                            idx = res_to;
                            res_ptr = variable(name);
                            break;
                            // throw ExprError(
                            //     "invalid token", tokens[idx],
                            //     "identifiers are treated as either a variable, or a constant name with following square brackets", tokens[identifier_hdr], tokens[idx - 1]);
                        }
                        name += tokens[idx].string();
                        break;
                    case TokenType::Spaces:
                        res_from = identifier_hdr;
                        res_to = idx - 1;
                        name_confirmed = true;
                        break;
                    default:
                        --idx;
                        if (!name_confirmed) {
                            res_from = identifier_hdr;
                            res_to = idx;
                        }
                        res_ptr = variable(name);
                        break;
                        // }
                        // throw ExprError(
                        //     "expected an open square bracket, got an invalid token", tokens[idx],
                        //     "during parsing a constant", tokens[identifier_hdr], tokens[idx - 1]);
                }
                if (res_ptr) break;
            }
            if (res_ptr) break;

            // std::cerr << "[debug] cname = \"" << name << "\"" << std::endl;
            std::cerr << "[debug] token after getting const name \"" << name << "\": " << tokens[idx] << std::endl;

            size_t pos_left = idx;
            incr_idx("an expr or a closing square bracket", "a constant", identifier_hdr);
            std::vector<std::shared_ptr<Term>> parameters;
            size_t pos_right = pos_left + 1, pos_comma = pos_left + 1;
            std::stack<TokenType> brackets;
            while (pos_right + 1 < end_of_token) {
                bool endwhile = false;
                auto ttype = tokens[pos_right].type();
                switch (ttype) {
                    case TokenType::SquareBracketLeft:
                        brackets.push(TokenType::SquareBracketRight);
                        break;
                    case TokenType::SquareBracketRight:
                        if (brackets.empty()) endwhile = true;
                        else if (brackets.top() == ttype) brackets.pop();
                        break;
                    default:
                        break;
                }
                if (endwhile) break;
                ++pos_right;
            }
            if (tokens[pos_right].type () != TokenType:: SquareBracketRight) {
                throw ExprError(
                    "closing square bracket not found", tokens[pos_right],
                    "square bracket opens here", tokens[idx]);
            }
            bool require_more_args = false;
            if (tokens[idx].type() == TokenType::Plus) {
                if (!flag_context) {
                    throw ExprError(
                        "got a dagger where no flag is given", tokens[idx],
                        "during parsing a constant", tokens[identifier_hdr], tokens[idx - 1]);
                }
                incr_idx("a comma or a closing square bracket", "a constant", identifier_hdr);
                if (tokens[idx].type() == TokenType::Comma) {
                    incr_idx("an expr", "a constant", identifier_hdr);
                    require_more_args = true;
                } else if (tokens[idx].type() != TokenType::SquareBracketRight) {
                    throw ExprError(
                        "expected a comma or a closing bracket, got an invalid token", tokens[idx],
                        "during parsing a constant", tokens[identifier_hdr], tokens[idx - 1]);
                }
                for (auto&& con : *flag_context) {
                    for (auto&& tv : *con) {
                        parameters.push_back(tv.value());
                    }
                }
            }
            if (require_more_args || tokens[idx].type() != TokenType::SquareBracketRight) {
                while (true) {
                    try {
                        parameters.emplace_back(parse_lambda(tokens, idx, end_of_token, flag_context)->term());
                    } catch (ExprError& e) {
                        ExprError newe(
                            "above error raised during parsing #" + std::to_string(parameters.size() + 1) + " expr of constant", tokens[idx],
                            "constant starts here", tokens[identifier_hdr]);
                        newe.chain(e);
                        throw newe;
                    }
                    incr_idx("a comma or a closing square bracket", "a constant", identifier_hdr);
                    if (tokens[idx].type() == TokenType::Comma) {
                        incr_idx("an expr", "a constant", identifier_hdr);
                    } else if (tokens[idx].type() == TokenType::SquareBracketRight) break;
                    else throw ExprError(
                        "expected an expr, got an invalid token", tokens[idx],
                        "during parsing a constant", tokens[identifier_hdr], tokens[idx - 1]);
                }
            }
            res_from = identifier_hdr;
            res_to = idx;
            res_ptr = constant(name, parameters);
            break;
        }
        default:
            throw ExprError("invalid token", tokens[idx]);
    }

    // std::cerr << "[debug] parsing identifier: token after parsing one expr: " << tokens[idx] << "\t";
    // if (res_ptr) std::cerr << "expr = " << res_ptr;
    // std::cerr << std::endl;

    if (idx + 1 < end_of_token && !no_chainer) {
        std::string chainer_name;
        size_t chain_hdr = idx + 1;
        incr_idx("a chainer", "an expr", chain_hdr, 1);
        switch (tokens[idx].type()) {
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
        // symbols handled below are treated as right associative
        if (chainer_name.size() > 0) {
            size_t chainer_pos = idx;
            // if (idx + 1 >= end_of_token) throw ExprError(
            //     "expected an expr after " + chainer_name + ", reached end of tokens", tokens[chainer_pos],
            //     "expr before " + chainer_name, tokens[res_from], tokens[res_to]);
            // ++idx;
            incr_idx("an expr after " + chainer_name, "an expr with chainer", chain_hdr);
            std::shared_ptr<ParseLambdaToken> plt;
            try {
                plt = parse_lambda(tokens, idx, flag_context);
            } catch (ExprError& e) {
                ExprError newe(
                    "above error raised during parsing expr", tokens[idx],
                    "after this " + chainer_name, tokens[chainer_pos]);
                newe.chain(e);
                throw newe;
            }
            std::shared_ptr<Term> expr = plt->term(), combined;
            switch (tokens[chainer_pos].type()) {
                case TokenType::Rightarrow:
                    // combined = pi(get_fresh_var(res_ptr, expr), res_ptr, expr);
                    combined = pi(get_fresh_var(expr), res_ptr, expr);
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
        } else {
            idx = chain_hdr - 1;
        }
    }
    if (res_to < 0){
        return std::make_shared<ParseLambdaToken>(tokens[res_from], res_ptr);
    }
    else {
        if (exhaust_token) {
            for (size_t i = idx + 1; i < end_of_token; ++i) {
                switch(tokens[i].type()) {
                    case TokenType::NewLine:
                    case TokenType::Spaces:
                        continue;
                    default:
                        throw ExprError("reached end of parse_lambda() (tokens must be exhausted)", tokens[idx]);
                }
            }
        }
        // std::cerr << "[debug] return: " << res_from << "-" << res_to << ", " << res_ptr << std::endl;
        return std::make_shared<ParseLambdaToken>(tokens[res_from], tokens[res_to], res_ptr);
    }
    throw ParseError("reached end of parse_lambda()", tokens[idx]);
}
std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx, size_t end_of_token, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context, bool no_chainer) {
    return parse_lambda(tokens, idx, end_of_token, false, flag_context, no_chainer);
}

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context, bool no_chainer) {
    return parse_lambda(tokens, idx, tokens.size(), flag_context, no_chainer);
}

std::vector<std::shared_ptr<FileData>> raw_string_fds;

std::shared_ptr<Term> parse_lambda(const std::string& str, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context) {
    size_t idx;
    std::shared_ptr<FileData> fdp = std::make_shared<FileData>(FileData({str}, "raw_string[parse_lambda]"));
    raw_string_fds.push_back(fdp);
    auto tokens = tokenize(*fdp);
    // for (size_t i = 0; i < tokens.size(); ++i) {
    //     std::cerr << "[debug] token " << i << ": " << tokens[i] << std::endl;
    // }
    return parse_lambda(tokens, idx = 0, tokens.size(), true, flag_context)->term();
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
        // if (DEBUG_CERR) std::cerr << "[debug; parse loop] flag[def]: " << def_str() << ", token: " << tokens[idx] << std::endl;
        if (DEBUG_CERR) {
            std::cerr << "[debug; parse loop] flag[def]: " << def_str() << ", flag[flg]: " << flg_str() << ", token: " << tokens[idx] << "\t";  // << (tokens[idx].type() == TokenType::NewLine ? "<NL>" : tokens[idx].string()) << "\"\t";
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
                        expr = parse_lambda(tokens, idx, std::make_shared<std::vector<std::shared_ptr<Context>>>(flag_context));
                    } catch (const ExprError& e) {
                        ParseError err("failed to parse a lambda expression", tokens[idx0]);
                        err.bind(e);
                        throw err;
                    }
                    // std::cerr << "[debug] read_lambda: " << to_string(*expr) << std::endl;
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
                case TokenType::Spaces:
                    continue;
                case TokenType::Period:
                    // std::cerr << "[debug] reached Period in parse_def() with read_lambda" << std::endl;
                    // [[fallthrough]];
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
                    if (flag_line_num < flag_context.size()) flag_context.resize(flag_line_num);
                    cname = t.string();
                    bool name_confirmed = false;
                    while (tokens[idx + 1].type() != TokenType::NewLine && tokens[idx + 1].type() != TokenType::DefinedBy) {
                        ++idx;
                        if (tokens[idx].type() == TokenType::Spaces) name_confirmed = true;
                        if (!name_confirmed) cname += tokens[idx].string();
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
                if (flag_line_num < flag_context.size()) flag_context.resize(flag_line_num);
                flag_context.push_back(std::make_shared<Context>());
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
            case TokenType::Spaces:
                continue;
            default:
                throw ParseError("not implemented in parse_defs() (token = " + to_string(t) + ")", t);
        }
    }
    return env;
}
