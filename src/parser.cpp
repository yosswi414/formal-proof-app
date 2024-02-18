#include "parser.hpp"

#include <deque>
#include <functional>
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

enum class ParseType {
    _WaitTerm,
    Paren_Open,
    Paren_Close,
    Term,
    Identifier_Str,
    Identifier_NumSym,
    Varname_Incomplete,
    Const_Name,
    Const_WaitTerm,
    Const_WaitCloseOrComma,
    Const_WaitTermOrClose,
    Const_Separator,
    Const_Open,
    Const_Close,
    Const_Context,
    Appl_WaitFirst,
    Appl_WaitSecond,
    AbstL_WaitVar,
    AbstL_WaitFirst,
    AbstL_WaitPeriod,
    AbstL_WaitSecond,
    AbstP_WaitVar,
    AbstP_WaitFirst,
    AbstP_WaitPeriod,
    AbstP_WaitSecond,
    Abst_SepColon,
    Abst_SepPeriod,
    Arrow_Kind,
    Arrow_Implies,
    Arrow_Equiv,
    Spaces,
    LineContinuator,
    EndOfLine,
    Undefined,
};

std::string to_string(const ParseType& ptype) {
    switch (ptype) {
        case ParseType::_WaitTerm: return "ParseType::_WaitTerm";
        case ParseType::Paren_Open: return "ParseType::Paren_Open";
        case ParseType::Paren_Close: return "ParseType::Paren_Close";
        case ParseType::Term: return "ParseType::Term";
        case ParseType::Identifier_Str: return "ParseType::Identifier_Str";
        case ParseType::Identifier_NumSym: return "ParseType::Identifier_NumSym";
        case ParseType::Varname_Incomplete: return "ParseType::Varname_Incomplete";
        case ParseType::Const_Name: return "ParseType::Const_Name";
        case ParseType::Const_WaitTerm: return "ParseType::Const_WaitTerm";
        case ParseType::Const_WaitCloseOrComma: return "ParseType::Const_WaitCloseOrComma";
        case ParseType::Const_WaitTermOrClose: return "ParseType::Const_WaitTermOrClose";
        case ParseType::Const_Separator: return "ParseType::Const_Separator";
        case ParseType::Const_Open: return "ParseType::Const_Open";
        case ParseType::Const_Close: return "ParseType::Const_Close";
        case ParseType::Const_Context: return "ParseType::Const_Context";
        case ParseType::Appl_WaitFirst: return "ParseType::Appl_WaitFirst";
        case ParseType::Appl_WaitSecond: return "ParseType::Appl_WaitSecond";
        case ParseType::AbstL_WaitVar: return "ParseType::AbstL_WaitVar";
        case ParseType::AbstL_WaitFirst: return "ParseType::AbstL_WaitFirst";
        case ParseType::AbstL_WaitPeriod: return "ParseType::AbstL_WaitPeriod";
        case ParseType::AbstL_WaitSecond: return "ParseType::AbstL_WaitSecond";
        case ParseType::AbstP_WaitVar: return "ParseType::AbstP_WaitVar";
        case ParseType::AbstP_WaitFirst: return "ParseType::AbstP_WaitFirst";
        case ParseType::AbstP_WaitPeriod: return "ParseType::AbstP_WaitPeriod";
        case ParseType::AbstP_WaitSecond: return "ParseType::AbstP_WaitSecond";
        case ParseType::Abst_SepColon: return "ParseType::Abst_SepColon";
        case ParseType::Abst_SepPeriod: return "ParseType::Abst_SepPeriod";
        case ParseType::Arrow_Kind: return "ParseType::Arrow_Kind";
        case ParseType::Arrow_Implies: return "ParseType::Arrow_Implies";
        case ParseType::Arrow_Equiv: return "ParseType::Arrow_Equiv";
        case ParseType::Spaces: return "ParseType::Spaces";
        case ParseType::LineContinuator: return "ParseType::LineContinuator";
        case ParseType::EndOfLine: return "ParseType::EndOfLine";
        case ParseType::Undefined: return "ParseType::Undefined";
    }
    return "[to_string(ParseType): not implemented (type value = " + std::to_string((int)ptype) + ")]";
}

std::string to_string_short(const ParseType& ptype) {
    switch (ptype) {
        case ParseType::_WaitTerm: return "_WaitT";
        case ParseType::Paren_Open: return "POp";
        case ParseType::Paren_Close: return "PCl";
        case ParseType::Term: return "T";
        case ParseType::Identifier_Str: return "IdS";
        case ParseType::Identifier_NumSym: return "IdN";
        case ParseType::Varname_Incomplete: return "Var";
        case ParseType::Const_Name: return "ConId";
        case ParseType::Const_WaitTerm: return "ConWT";
        case ParseType::Const_WaitCloseOrComma: return "ConWCC";
        case ParseType::Const_WaitTermOrClose: return "ConWTC";
        case ParseType::Const_Separator: return "ConSp";
        case ParseType::Const_Open: return "ConOp";
        case ParseType::Const_Close: return "ConCl";
        case ParseType::Const_Context: return "Con+";
        case ParseType::Appl_WaitFirst: return "Appl1";
        case ParseType::Appl_WaitSecond: return "Appl2";
        case ParseType::AbstL_WaitVar: return "AbLWV";
        case ParseType::AbstL_WaitFirst: return "AbL1";
        case ParseType::AbstL_WaitPeriod: return "AbLp";
        case ParseType::AbstL_WaitSecond: return "AbL2";
        case ParseType::AbstP_WaitVar: return "AbPWV";
        case ParseType::AbstP_WaitFirst: return "AbP1";
        case ParseType::AbstP_WaitPeriod: return "AbPp";
        case ParseType::AbstP_WaitSecond: return "AbP2";
        case ParseType::Abst_SepColon: return "AbSpCl";
        case ParseType::Abst_SepPeriod: return "AbSpPr";
        case ParseType::Arrow_Kind: return "ArrK";
        case ParseType::Arrow_Implies: return "ArrI";
        case ParseType::Arrow_Equiv: return "ArrE";
        case ParseType::Spaces: return "Spc";
        case ParseType::LineContinuator: return "LC";
        case ParseType::EndOfLine: return "EOL";
        case ParseType::Undefined: return "UD";
    }
    return "[to_string_short(ParseType): not implemented (type value = " + std::to_string((int)ptype) + ")]";
}

class ParseStack {
  public:
    ParseStack() : ParseStack(ParseType::_WaitTerm, 0) {}
    ParseStack(TokenType type, int idx) : _token_begin(idx), _token_end(idx + 1) {
        switch (type) {
            case TokenType::String:
            case TokenType::Character:
            case TokenType::Underscore:
                _ptype = ParseType::Identifier_Str;
                break;
            default:
                _ptype = ParseType::Identifier_NumSym;
        }
    }
    ParseStack(ParseType type, int idx) : _ptype(type), _token_begin(idx), _token_end(idx + 1) {}
    ParseStack(ParseType type, int begin, int end) : _ptype(type), _token_begin(begin), _token_end(end) {}
    ParseStack(const std::shared_ptr<Term> term, int idx) : _ptype(ParseType::Term), _token_begin(idx), _token_end(idx + 1), _terms({term}) {}

    ParseType ptype() const { return _ptype; }
    void change_ptype(ParseType nt) { _ptype = nt; }

    std::string excerpt(const std::vector<Token>& tokens) const {
        if (_token_begin < 0) return "(n/a)";
        std::string res;
        for (int i = _token_begin; i < _token_end; ++i) res += tokens[i].string();
        return res;
    }
    int begin() const { return _token_begin; }
    int& begin() { return _token_begin; }
    int end() const { return _token_end; }
    int& end() { return _token_end; }

    std::vector<std::shared_ptr<Term>>& terms() { return _terms; }
    const std::vector<std::shared_ptr<Term>>& terms() const { return _terms; }
    void add_term(const std::shared_ptr<Term> term) { _terms.push_back(term); }

  private:
    ParseType _ptype;
    int _token_begin = 0, _token_end = 0;
    std::vector<std::shared_ptr<Term>> _terms;
};

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx, size_t end_of_token, bool exhaust_token, const std::vector<std::shared_ptr<Context>>& flag_context, const Environment& definitions) {
    const size_t pos_init = idx;  // for rollback use

    using stk_t = std::stack<ParseStack>;
    using stk_pt = std::shared_ptr<stk_t>;

    using abstv_t = std::deque<std::shared_ptr<Term>>;
    using abstv_pt = std::shared_ptr<abstv_t>;

    std::queue<std::pair<stk_pt, abstv_pt>> tree;
    tree.emplace(std::make_shared<stk_t>(), std::make_shared<abstv_t>());

    stk_pt stk = tree.front().first;
    stk->emplace();

    /* successful condition:
        - AND:
            - stk has one Term
            - OR:
                - reaches EndOfLine without LineContinuation
                - reaches invalid token
                - [rejected] reads another Term (stk = [Term Term])
        return:
            - the first Term
        note:
            - here we will interpret exhaust_token = true as:
                - supposed to be able to read all tokens from pos_init to end_of_token
                - throw an exception as soon as any invalid token is found in the range
            - exhaust_token = false as:
                - return the last Term if any invalid token is found
    */

    std::set<std::string> defined_names;
    for (auto&& def : definitions) defined_names.insert(def->definiendum());
    auto exists_cand_const = [&defined_names](const std::string& cname_part) {
        auto itr = std::lower_bound(defined_names.begin(), defined_names.end(), cname_part);
        if (itr == defined_names.end()) return false;
        return std::mismatch(cname_part.begin(), cname_part.end(), itr->begin(), itr->end()).first == cname_part.end();
        // return itr->starts_with(cname_part);
    };

    std::stack<ParseType> atmosphere;
    atmosphere.push(ParseType::Undefined);

    abstv_pt abst_vars = tree.front().second;

    auto is_arrow = [](ParseType type) {
        switch (type) {
            case ParseType::Arrow_Kind:
            case ParseType::Arrow_Implies:
            case ParseType::Arrow_Equiv:
                return true;
            default:
                return false;
        }
    };

    auto is_readable_as_term = [](ParseType type) {
        switch (type) {
            case ParseType::Term:
            case ParseType::Identifier_Str:
            case ParseType::Varname_Incomplete:
                return true;
            default:
                return false;
        }
    };

    auto is_period_nameonly_stktop = [](ParseType type) {
        switch (type) {
            // Q. is period in [type] . impossible to be a separator? yes -> true.
            // equivalent question: is [type] possible to be the end of expr? no -> true.
            case ParseType::Arrow_Kind:
            case ParseType::Arrow_Implies:
            case ParseType::Arrow_Equiv:
            case ParseType::_WaitTerm:
            case ParseType::Paren_Open:
            case ParseType::Const_Name:
            case ParseType::Const_WaitTerm:
            case ParseType::Const_WaitCloseOrComma:
            case ParseType::Const_WaitTermOrClose:
            case ParseType::Const_Separator:
            case ParseType::Const_Open:
            case ParseType::Const_Context:
            case ParseType::Appl_WaitFirst:
            case ParseType::Appl_WaitSecond:
            case ParseType::AbstL_WaitVar:
            case ParseType::AbstL_WaitFirst:
            case ParseType::AbstL_WaitSecond:
            case ParseType::AbstP_WaitVar:
            case ParseType::AbstP_WaitFirst:
            case ParseType::AbstP_WaitSecond:
            case ParseType::Abst_SepColon:
            case ParseType::Abst_SepPeriod:
                return true;

            case ParseType::Paren_Close:
            case ParseType::Term:
            case ParseType::Varname_Incomplete:
            case ParseType::Identifier_Str:
            case ParseType::Identifier_NumSym:
            case ParseType::Const_Close:
            case ParseType::Spaces:
                return false;

            default:
                break;
        }
        return false;
    };

    auto is_period_nameonly = [&is_period_nameonly_stktop](ParseType absorber_type, ParseType stack_top_type) {
        switch (absorber_type) {
            case ParseType::AbstL_WaitVar:
            case ParseType::AbstP_WaitVar:
                return true;
            default:
                break;
        }
        return is_period_nameonly_stktop(stack_top_type);
    };

    bool end_of_line = false;
    std::shared_ptr<ParseError> invalid_token_err;

    auto type_seq_to_str = [](const std::deque<ParseStack>& stash) {
        std::string type_seq_str = "[";
        size_t max_len = std::min(30ul, stash.size());
        type_seq_str += to_string(stash[0].ptype());
        for (size_t i = 1; i < max_len; ++i) type_seq_str += ", " + to_string(stash[i].ptype());
        if (max_len < stash.size()) type_seq_str += ", ...";
        type_seq_str += "](len = " + std::to_string(stash.size()) + ")";
        return type_seq_str;
    };

    auto stack_dump = [&tokens](stk_pt& stk) {
        std::deque<ParseStack> stash;
        size_t size_orig = stk->size();
        size_t max_len = std::min(6ul, size_orig);

        for (size_t i = 0; !stk->empty() && i < max_len; ++i) {
            stash.push_front(stk->top());
            stk->pop();
        }
        std::string expr_seq_str = "[";
        if (max_len < size_orig) expr_seq_str += "..., ";
        expr_seq_str += "\"" + stash[0].excerpt(tokens) + "\"(" + to_string_short(stash[0].ptype()) + ")";
        for (size_t i = 1; i < max_len; ++i) expr_seq_str += ", \"" + stash[i].excerpt(tokens) + "\"(" + to_string_short(stash[i].ptype()) + ")";
        expr_seq_str += "](len = " + std::to_string(size_orig) + ")";
        while (!stash.empty()) {
            stk->push(stash.front());
            stash.pop_front();
        }
        return expr_seq_str;
    };

    bool got_term = false;

    while (true) {
        // fetch token
        switch (tokens[idx].type()) {
            case TokenType::Asterisk:
                stk->emplace(star, idx);
                break;
            case TokenType::AtSign:
                stk->emplace(sq, idx);
                break;
            case TokenType::Character:
            case TokenType::String:
            case TokenType::Underscore:
                stk->emplace(ParseType::Identifier_Str, idx);
                break;
            case TokenType::Number:
            case TokenType::Hyphen:
                stk->emplace(ParseType::Identifier_NumSym, idx);
                break;
            case TokenType::Period: {
                // stk->emplace(ParseType::Identifier_NumSym, idx);
                std::string name = stk->top().excerpt(tokens);
                // if Identifier + '.' is still a prefix of a constant, treat '.' as part of identifier
                ParseType main_type, branch_type;
                bool needs_branch = true;
                if (atmosphere.size() == 1 || is_period_nameonly(atmosphere.top(), stk->top().ptype())) {
                    needs_branch = false;
                    main_type = ParseType::Identifier_NumSym;
                } else if (stk->top().ptype() == ParseType::Identifier_Str && exists_cand_const(name + ".")) {
                    main_type = ParseType::Identifier_NumSym;
                    branch_type = ParseType::Abst_SepPeriod;
                } else {
                    main_type = ParseType::Abst_SepPeriod;
                    branch_type = ParseType::Identifier_NumSym;
                }
                stk->emplace(main_type, idx);

                if (needs_branch) {
                    stk_pt branch = std::make_shared<stk_t>(*stk);
                    branch->emplace(branch_type, idx);
                    tree.emplace(branch, std::make_shared<abstv_t>(*abst_vars));
                }
                break;
            }
            case TokenType::Spaces:
                stk->emplace(ParseType::Spaces, idx);
                break;
            case TokenType::Colon:
                stk->emplace(ParseType::Abst_SepColon, idx);
                break;
            case TokenType::Percent:
                stk->emplace(ParseType::Appl_WaitFirst, idx);
                break;
            case TokenType::DollarSign:
                stk->emplace(ParseType::AbstL_WaitVar, idx);
                break;
            case TokenType::QuestionMark:
                stk->emplace(ParseType::AbstP_WaitVar, idx);
                break;
            case TokenType::ParenLeft:
                stk->emplace(ParseType::Paren_Open, idx);
                break;
            case TokenType::ParenRight:
                stk->emplace(ParseType::Paren_Close, idx);
                break;
            case TokenType::SquareBracketLeft:
                stk->emplace(ParseType::Const_Open, idx);
                break;
            case TokenType::SquareBracketRight:
                stk->emplace(ParseType::Const_Close, idx);
                break;
            case TokenType::Comma:
                stk->emplace(ParseType::Const_Separator, idx);
                break;
            case TokenType::Plus:
                stk->emplace(ParseType::Const_Context, idx);
                break;
            case TokenType::Leftrightdoublearrow:
                stk->emplace(ParseType::Arrow_Equiv, idx);
                break;
            case TokenType::Rightarrow:
                stk->emplace(ParseType::Arrow_Kind, idx);
                break;
            case TokenType::Rightdoublearrow:
                stk->emplace(ParseType::Arrow_Implies, idx);
                break;
            case TokenType::Backslash:
                stk->emplace(ParseType::LineContinuator, idx);
                break;
            case TokenType::NewLine:
                stk->emplace(ParseType::EndOfLine, idx);
                break;
            default:
                stk->emplace(ParseType::Undefined, idx);
                break;
        }

        // find contractable series of tokens
        std::deque<ParseStack> stash;

        auto load = [&stk, &stash](int cnt) {
            while (cnt--) {
                stash.push_front(stk->top());
                stk->pop();
            }
        };

        auto flush = [&stk, &stash](int cnt = 0) {
            while (!stash.empty()) {
                if (--cnt >= 0) stk->push(stash.front());
                stash.pop_front();
            }
        };

        auto load_arrow_seq = [&load, &is_readable_as_term, &is_arrow, &stk](std::deque<ParseStack>& stash) {
            while (!stk->empty() &&
                   (stash.size() % 2
                        ? is_arrow(stk->top().ptype())
                        : is_readable_as_term(stk->top().ptype()))) { load(1); }
        };

        auto elim_right_assoc = [&invalid_token_err, &flush, &tokens, &type_seq_to_str](std::deque<ParseStack>& stash) -> bool {
            std::vector<ParseType> order{
                ParseType::Arrow_Kind,
                ParseType::Arrow_Implies,
                ParseType::Arrow_Equiv,
            };
            bool reduced = false;
            // normalize (identifier between arrows -> variable)
            for (size_t i = 0; i < stash.size(); ++i) {
                switch (stash[i].ptype()) {
                    case ParseType::Identifier_Str:
                    case ParseType::Varname_Incomplete:
                        stash[i].terms() = {variable(stash[i].excerpt(tokens))};
                        stash[i].change_ptype(ParseType::Term);
                        reduced = true;
                        break;
                    case ParseType::Term:
                    case ParseType::Arrow_Kind:
                    case ParseType::Arrow_Implies:
                    case ParseType::Arrow_Equiv:
                        break;
                    default:
                        invalid_token_err = std::make_shared<ParseError>(
                            "Invalid token found. Type: " + to_string(stash[i].ptype()),
                            tokens[stash[i].begin()], tokens[stash[i].end() - 1],
                            "In term-arrow sequence",
                            tokens[stash.front().begin()], tokens[stash.back().end() - 1]);
                        flush(i - 1);
                        return false;
                }
            }
            for (auto arrow_t : order) {
                for (int i = stash.size() - 1; i >= 0; --i) {
                    if (stash[i].ptype() != arrow_t) continue;
                    if (i + 1ul >= stash.size()) {
                        throw ParseError(
                            "Expected a term in right hand side of the operator, reached end of expr",
                            tokens[stash[i].begin()]);
                    }
                    if (stash[i + 1].ptype() != ParseType::Term) {
                        throw ParseError(
                            "Expected a term in right hand side of the operator, got a non-term object. Type: " + to_string(stash[i + 1].ptype()),
                            tokens[stash[i + 1].begin()], tokens[stash[i + 1].end() - 1],
                            "The operator locates here",
                            tokens[stash[i].begin()]);
                    }
                    if (i - 1 < 0) {
                        throw ParseError(
                            "Expected a term in left hand side of the operator, reached beginning of expr",
                            tokens[stash[i].begin()]);
                    }
                    if (stash[i - 1].ptype() != ParseType::Term) {
                        throw ParseError(
                            "Expected a term in left hand side of the operator, got a non-term object. Type: " + to_string(stash[i - 1].ptype()),
                            tokens[stash[i - 1].begin()], tokens[stash[i - 1].end() - 1],
                            "The operator locates here",
                            tokens[stash[i].begin()]);
                    }

                    // T arrow T -> T
                    std::shared_ptr<Term> T1, T2, term_new;
                    T1 = stash[i - 1].terms().front();
                    T2 = stash[i + 1].terms().front();
                    switch (arrow_t) {
                        case ParseType::Arrow_Kind:
                            term_new = pi(get_fresh_var(T2), T1, T2);
                            break;
                        case ParseType::Arrow_Implies:
                            term_new = constant("implies", {T1, T2});
                            break;
                        case ParseType::Arrow_Equiv:
                            term_new = constant("equiv", {T1, T2});
                            break;
                        default:
                            throw ParseError(
                                "unreachable (this is a bug)",
                                tokens[stash[i].begin()]);
                    }
                    stash[i - 1].terms() = {term_new};
                    stash[i - 1].end() = stash[i + 1].end();
                    stash.erase(stash.begin() + i + 1);
                    stash.erase(stash.begin() + i);
                    --i;
                    reduced = true;
                }
            }
            if (stash.front().ptype() != ParseType::Term) {
                throw ParseError(
                    "Could not reduce term-arrow sequence in parentheses to a single term",
                    tokens[stash.front().begin()], tokens[stash.back().end() - 1],
                    "The type of the result of reduction is not ParseType::Term. Type: " + to_string(stash.front().ptype()),
                    tokens[stash.front().begin()], tokens[stash.back().end() - 1]);
            }
            if (stash.size() != 1) {
                std::string type_seq_str = type_seq_to_str(stash);
                invalid_token_err = std::make_shared<ParseError>(
                    "Could not reduce term-arrow sequence in parentheses to a single term",
                    tokens[stash.front().begin()], tokens[stash.back().end() - 1],
                    "At least one irreducible token exists. Types: " + type_seq_str,
                    tokens[stash.front().begin()], tokens[stash.back().end() - 1]);
            }
            return reduced;
        };

        auto contract_top_term = [&abst_vars, &tokens, &stack_dump, &got_term](stk_pt stk, bool is_term_decided = false) -> bool {
            bool reduced = false;

            // debug("contract_top_term(): " << fstr(is_term_decided) << ", stack = " << stack_dump(stk));
            if (is_term_decided) {
                switch (stk->top().ptype()) {
                    case ParseType::Term:
                        break;
                    case ParseType::Identifier_Str:
                    case ParseType::Varname_Incomplete: {
                        stk->top().change_ptype(ParseType::Term);
                        stk->top().terms() = {variable(stk->top().excerpt(tokens))};
                        reduced = true;
                        break;
                    }
                    default:
                        break;
                }
            }

            while (stk->size() >= 2 && stk->top().ptype() == ParseType::Term) {
                ParseStack s1 = stk->top();
                stk->pop();
                ParseStack s0 = stk->top();
                stk->pop();
                // debug("contract_top_term(): s0, s1 = " << to_string_short(s0.ptype()) << ", " << to_string_short(s1.ptype()));
                bool is_lambda = false;
                switch (s0.ptype()) {
                    case ParseType::_WaitTerm: {
                        stk->push(s1);
                        reduced = true;
                        got_term = true;
                        break;
                    }
                    // for simplicity, it never takes further arrow in appl and second expr of abst
                    // unless they are given inside of parentheses
                    case ParseType::AbstL_WaitSecond:
                        is_lambda = true;
                        [[fallthrough]];
                    case ParseType::AbstP_WaitSecond: {
                        const auto var = s0.terms()[0];
                        const auto type = s0.terms()[1];
                        const auto expr = s1.terms()[0];
                        if (is_lambda) s0.terms() = {lambda(var, type, expr)};
                        else s0.terms() = {pi(var, type, expr)};
                        abst_vars->pop_back();
                        s0.change_ptype(ParseType::Term);
                        s0.end() = s1.end();
                        stk->push(s0);
                        reduced = true;
                        break;
                    }
                    case ParseType::Appl_WaitFirst: {
                        const auto term = s1.terms()[0];
                        s0.terms() = {term};
                        s0.change_ptype(ParseType::Appl_WaitSecond);
                        s0.end() = s1.end();
                        stk->push(s0);
                        reduced = true;
                        break;
                    }
                    case ParseType::Appl_WaitSecond: {
                        const auto term2 = s1.terms()[0];
                        const auto term1 = s0.terms()[0];
                        s0.terms() = {appl(term1, term2)};
                        s0.change_ptype(ParseType::Term);
                        s0.end() = s1.end();
                        stk->push(s0);
                        reduced = true;
                        break;
                    }
                    case ParseType::Term: {
                        // if (!is_term_decided) {
                        if (true) {
                            stk->push(s0);
                            stk->push(s1);
                            return reduced;
                        };
                        // application without explicit symbol '%'?
                        const auto term1 = s0.terms()[0];
                        const auto term2 = s1.terms()[0];
                        size_t end = s1.end();
                        s0.terms() = {appl(term1, term2)};
                        s0.change_ptype(ParseType::Term);
                        s0.end() = end;
                        stk->push(s0);
                        reduced = true;
                        break;
                    }
                    case ParseType::AbstL_WaitFirst:
                        is_lambda = true;
                        [[fallthrough]];
                    case ParseType::AbstP_WaitFirst: {
                        if (!is_term_decided) {
                            stk->push(s0);
                            stk->push(s1);
                            return reduced;
                        }
                        // debug("contract_top_term():AbP1: " << fstr(s0.terms().size()));
                        // debug("contract_top_term():AbP1: " << fstr(s0.terms().front()));
                        // debug("contract_top_term():AbP1: " << fstr(s1.terms().size()));
                        // debug("contract_top_term():AbP1: " << fstr(s1.terms().front()));
                        // debug("contract_top_term():AbP1: s0 bound: " << s0.terms()[0]);
                        const auto term = s1.terms()[0];
                        // debug("contract_top_term():AbP1: " << fstr(term));
                        // debug("contract_top_term():AbP1: term type = " << to_string(term->etype()));
                        s0.add_term(term);
                        ParseType new_type = (is_lambda ? ParseType::AbstL_WaitPeriod : ParseType::AbstP_WaitPeriod);
                        s0.change_ptype(new_type);
                        s0.end() = s1.end();
                        stk->push(s0);
                        reduced = true;
                        break;
                    }
                    case ParseType::Const_WaitTermOrClose:
                    case ParseType::Const_WaitTerm: {
                        if (!is_term_decided) {
                            stk->push(s0);
                            stk->push(s1);
                            return reduced;
                        }
                        const auto term = s1.terms()[0];
                        s0.add_term(term);
                        s0.change_ptype(ParseType::Const_WaitCloseOrComma);
                        s0.end() = s1.end();
                        stk->push(s0);
                        reduced = true;
                        break;
                    }
                    default:
                        stk->push(s0);
                        stk->push(s1);
                        return reduced;
                }
                // debug("contract_top_term(): one step, stack = " << stack_dump(stk));
            }
            // debug("contract_top_term(): finished: " << stk->top().terms()[0]);
            return reduced;
        };

        bool reduced = true;

        // debug(fstr(idx) << " < " << end_of_token << ", fetched: " << tokens[idx] << " (" << to_string(stk->top().ptype()) << ")");

        invalid_token_err.reset();
        while (!invalid_token_err && !end_of_line && reduced) {
            reduced = false;
            switch (stk->top().ptype()) {
                // tokens reactive at front
                case ParseType::Undefined:
                    invalid_token_err = std::make_shared<ParseError>(
                        "Invalid token found",
                        tokens[idx]);
                    break;
                case ParseType::Identifier_Str:
                case ParseType::Identifier_NumSym: {
                    load(2);
                    // Identifier_Str Identifier_* -> Identifier_Str
                    if (stash[0].ptype() == ParseType::Identifier_Str) {
                        stash[0].end() = stash[1].end();
                        if (!exists_cand_const(stash[0].excerpt(tokens))) stash[0].change_ptype(ParseType::Varname_Incomplete);
                        flush(1);
                        reduced = true;
                    } else if (stash[0].ptype() == ParseType::Varname_Incomplete) {
                        stash[0].end() = stash[1].end();
                        flush(1);
                        reduced = true;
                    }
                    // (not Identifier_Str) Identifier_NumSym : Error
                    else if (stash[1].ptype() == ParseType::Identifier_NumSym) {
                        invalid_token_err = std::make_shared<ParseError>(
                            "An identifier must begin with alphabet or underscore",
                            tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                        flush(1);
                        break;
                    }
                    flush(2);
                    break;
                }

                case ParseType::Const_Open: {
                    load(2);
                    std::string name = stash[0].excerpt(tokens);
                    switch (stash[0].ptype()) {
                        // only valid
                        case ParseType::Identifier_Str: {
                            // undefined constant : error
                            if (definitions.lookup_index(name) >= 0) {
                                stash[0].change_ptype(ParseType::Const_Name);
                                stash[1].change_ptype(ParseType::Const_WaitTermOrClose);
                                flush(2);
                                reduced = true;
                                atmosphere.push(ParseType::Const_Open);
                                break;
                            }
                            [[fallthrough]];
                        }
                        case ParseType::Varname_Incomplete:
                            throw ParseError(
                                "The name \"" + name + "\" is not defined as a constant",
                                tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                        case ParseType::Identifier_NumSym:
                            throw ParseError(
                                "An identifier must begin with alphabet or underscore",
                                tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                        default:
                            throw ParseError(
                                "A defined identifier is missing before square bracket",
                                tokens[stash[1].begin()], tokens[stash[1].end() - 1]);
                    }
                    break;
                }

                case ParseType::Spaces: {
                    load(2);
                    if (stash[0].ptype() == ParseType::Identifier_Str) {
                        std::string name = stash[0].excerpt(tokens);
                        ParseType newtype = ParseType::Const_Name;
                        if (definitions.lookup_index(name) < 0) {
                            newtype = ParseType::Term;
                            stash[0].terms() = {variable(name)};
                        }
                        stash[0].change_ptype(newtype);
                    }
                    flush(1);
                    reduced = true;
                    break;
                }

                case ParseType::Paren_Close: {
                    ParseStack close = stk->top();
                    stk->pop();
                    reduced |= contract_top_term(stk, true);
                    stk->push(close);
                    while (!stk->empty() && stk->top().ptype() != ParseType::Paren_Open) load(1);
                    if (stk->empty()) {
                        throw ParseError(
                            "cannot close parenthesis that is not opened",
                            tokens[stash.back().begin()], tokens[stash.back().end() - 1]);
                    }
                    load(1);  // load '('
                    // debug(fstr(idx) << ", at PCl, stash = " << type_seq_to_str(stash));
                    int begin = stash.front().begin(), end = stash.back().end();
                    // strip parentheses
                    stash.pop_front();
                    stash.pop_back();
                    // eliminate arrows
                    elim_right_assoc(stash);
                    stash.front().begin() = begin;
                    stash.front().end() = end;
                    flush(1);

                    reduced = true;
                    break;
                }

                case ParseType::EndOfLine: {
                    load(2);
                    if (stash[0].ptype() == ParseType::LineContinuator) {
                        flush();
                        reduced = true;
                        break;
                    }
                    // end of expr
                    // check if we get a single ParseType::Term in stk
                    // or spill out some invalid tokens left in stk
                    flush(1);  // strip EndOfLine
                    reduced = true;
                    end_of_line = true;
                    break;
                }

                case ParseType::Const_Separator: {
                    if (atmosphere.top() != ParseType::Const_Open) {
                        invalid_token_err = std::make_shared<ParseError>(
                            "Invalid token (comma can only be used as a separator in constant argument list)",
                            tokens[stk->top().begin()]);
                        break;
                    }
                    // Const_WaitTerm T arr ... arr T ,
                    ParseStack comma = stk->top();
                    stk->pop();
                    if (is_readable_as_term(stk->top().ptype())) {
                        load_arrow_seq(stash);
                        elim_right_assoc(stash);
                        flush(1);
                        contract_top_term(stk, true);
                    }
                    stk->push(comma);

                    // Const_WaitCloseOrComma ,
                    load(2);
                    switch (stash[0].ptype()) {
                        case ParseType::Const_WaitTermOrClose:
                        case ParseType::Const_WaitTerm: {
                            size_t pos_arg = stash[0].terms().size() + 1;
                            while (!stk->empty() && stk->top().ptype() != ParseType::Const_Name) stk->pop();
                            if (stk->empty()) {
                                throw ParseError(
                                    "Constant name not found before this comma (this is a bug)",
                                    tokens[stash[0].begin()]);
                            }
                            throw ParseError(
                                "Argument #" + std::to_string(pos_arg) + " is empty",
                                tokens[stash[1].begin()], tokens[stash[1].end() - 1],
                                "Of constant",
                                tokens[stk->top().begin()], tokens[stk->top().end() - 1]);
                        }
                        case ParseType::Const_WaitCloseOrComma: {
                            // stash[0].add_term(stash[1].terms().front());
                            stash[0].change_ptype(ParseType::Const_WaitTerm);
                            stash[0].end() = stash[1].end();
                            flush(1);
                            // Const_WaitCloseOrComma
                            reduced = true;
                            break;
                        }
                        default:
                            invalid_token_err = std::make_shared<ParseError>(
                                "Invalid token (comma can only be used as a separator in constant argument list)",
                                tokens[stash[1].begin()], tokens[stash[1].end() - 1]);
                            flush(1);
                            break;
                    }
                    break;
                }

                case ParseType::Const_Close: {
                    if (atmosphere.top() != ParseType::Const_Open) {
                        invalid_token_err = std::make_shared<ParseError>(
                            "Invalid token (closing square bracket can only be used as the end of argument list of constant)",
                            tokens[stk->top().begin()]);
                        break;
                    }
                    if (invalid_token_err) break;
                    // Const_Name Const_WaitTerm T ]
                    ParseStack close = stk->top();
                    stk->pop();
                    if (is_readable_as_term(stk->top().ptype())) {
                        load_arrow_seq(stash);
                        elim_right_assoc(stash);
                        flush(1);
                        contract_top_term(stk, true);
                    }
                    stk->push(close);

                    // Const_Name Const_WaitCloseOrComma ]
                    load(3);
                    if (stash[0].ptype() != ParseType::Const_Name) {
                        throw ParseError(
                            "This should be a constant name (this is a bug)",
                            tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                    }
                    switch (stash[1].ptype()) {
                        case ParseType::Const_WaitTermOrClose:
                        case ParseType::Const_WaitCloseOrComma: {
                            int end = stash[2].end();
                            const std::string& cname = stash[0].excerpt(tokens);
                            const auto& args = stash[1].terms();
                            // argc match check
                            size_t argc = definitions.lookup_def(cname)->context()->size();
                            if (argc != args.size()) {
                                throw ParseError(
                                    "The length of argument list "
                                    "(got: " +
                                        std::to_string(args.size()) + ") "
                                                                      "doesn't match the context length of the defined constant \"" +
                                        cname + "\" "
                                                "(expected: " +
                                        std::to_string(argc) + ")",
                                    tokens[stash[1].begin()], tokens[stash[2].end() - 1]);
                            }
                            stash[0].end() = end;
                            stash[0].change_ptype(ParseType::Term);
                            stash[0].terms() = {constant(cname, args)};
                            // T
                            flush(1);
                            atmosphere.pop();
                            reduced = true;
                            break;
                        }
                        case ParseType::Const_WaitTerm: {
                            size_t pos_arg = stash[1].terms().size() + 1;
                            throw ParseError(
                                "Argument #" + std::to_string(pos_arg) + " is empty. Remove comma or put another term before closing",
                                tokens[stash[1].begin()], tokens[stash[1].end() - 1],
                                "Of constant",
                                tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                        }
                        default: {
                            invalid_token_err = std::make_shared<ParseError>(
                                "Invalid token (closing square bracket can only be used as the end of argument list of constant)",
                                tokens[stash[2].begin()], tokens[stash[2].end() - 1]);
                            flush(2);
                            break;
                        }
                    }
                    break;
                }

                case ParseType::Const_Context: {
                    load(2);
                    switch (stash[0].ptype()) {
                        case ParseType::Const_WaitTermOrClose:
                        case ParseType::Const_WaitTerm: {
                            int end = stash[1].end();
                            for (auto&& tvs : flag_context) {
                                for (auto&& tv : *tvs) stash[0].add_term(tv.value());
                            }
                            stash[0].end() = end;
                            stash[0].change_ptype(ParseType::Const_WaitCloseOrComma);
                            flush(1);
                            reduced = true;
                            break;
                        }
                        default:
                            throw ParseError(
                                "Invalid token (plus symbol can only be used as the abbreviation of context for argument of constant)",
                                tokens[stash[1].begin()], tokens[stash[1].end() - 1]);
                    }
                    break;
                }

                case ParseType::Abst_SepColon: {
                    // Abst_WaitVar Identifier Abst_SepColon
                    // debug(fstr(idx) << ", at AbSpCl, absorber = " << to_string(atmosphere.top()) << ", stack = " << stack_dump(stk));

                    switch (atmosphere.top()) {
                        case ParseType::AbstL_WaitVar:
                        case ParseType::AbstP_WaitVar:
                            break;
                        default:
                            invalid_token_err = std::make_shared<ParseError>(
                                "Invalid token (colon can only be used as a separator between bound variable and its type in expr)",
                                tokens[stk->top().begin()]);
                            break;
                    }
                    if (invalid_token_err) break;
                    load(3);
                    bool is_lambda = false;
                    switch (stash[0].ptype()) {
                        case ParseType::AbstL_WaitVar:
                            is_lambda = true;
                            [[fallthrough]];
                        case ParseType::AbstP_WaitVar: {
                            // debug("at AbSpCl, stash[1].ptype() = " << to_string(stash[1].ptype()));
                            switch (stash[1].ptype()) {
                                case ParseType::Varname_Incomplete:
                                case ParseType::Identifier_Str:
                                    stash[1].change_ptype(ParseType::Term);
                                    stash[1].terms() = {variable(stash[1].excerpt(tokens))};
                                    [[fallthrough]];
                                case ParseType::Term: {
                                    auto var = stash[1].terms().front();
                                    int end = stash[2].end();
                                    if (var->etype() != EpsilonType::Variable) {
                                        throw ParseError(
                                            "Expected a bound variable, got a non-variable term",
                                            tokens[stash[1].begin()], tokens[stash[1].end() - 1],
                                            "During parsing an abstraction",
                                            tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                                    }
                                    ParseType wait_t = is_lambda ? ParseType::AbstL_WaitFirst : ParseType::AbstP_WaitFirst;
                                    stash[0].change_ptype(wait_t);
                                    stash[0].end() = end;
                                    stash[0].terms() = {var};
                                    abst_vars->push_back(var);
                                    flush(1);
                                    reduced = true;

                                    atmosphere.pop();
                                    atmosphere.push(wait_t);
                                    break;
                                }
                                default:
                                    throw ParseError(
                                        "Expected a bound variable, got an invalid token",
                                        tokens[stash[1].begin()], tokens[stash[1].end() - 1],
                                        "During parsing an abstraction",
                                        tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                            }
                            break;
                        }
                        default:
                            // debug(fstr(stash.size()));
                            // debug("ptype: " << to_string(stash[0].ptype()));
                            // debug(fstr(stash[0].end()));
                            // debug(fstr(tokens[stash[0].begin()]));
                            invalid_token_err = std::make_shared<ParseError>(
                                "Invalid token (colon can only be used as a separator between bound variable and its type in expr)",
                                tokens[stash[0].begin()]);
                            flush(2);
                            break;
                    }
                    break;
                }

                case ParseType::Abst_SepPeriod: {
                    // Abst_WaitFirst Term arrow ... arrow Term Abst_SepPeriod
                    ParseStack period = stk->top();
                    stk->pop();

                    load_arrow_seq(stash);
                    reduced |= elim_right_assoc(stash);
                    flush(1);
                    // debug(fstr(idx) << ", at AbSpPr after elim right assoc, stack = " << stack_dump(stk));
                    reduced |= contract_top_term(stk, true);
                    // debug(fstr(idx) << ", at AbSpPr after contract top term, stack = " << stack_dump(stk));
                    stk->push(period);

                    // Abst_WaitFirst Term Abst_SepPeriod

                    load(2);
                    bool is_lambda = false;
                    switch (stash[0].ptype()) {
                        case ParseType::AbstL_WaitPeriod:
                            is_lambda = true;
                            [[fallthrough]];
                        case ParseType::AbstP_WaitPeriod: {
                            ParseType wait_t = is_lambda ? ParseType::AbstL_WaitSecond : ParseType::AbstP_WaitSecond;
                            int end = stash[1].end();
                            stash[0].change_ptype(wait_t);
                            stash[0].end() = end;
                            flush(1);
                            atmosphere.pop();
                            reduced = true;
                            break;
                        }
                        default:
                            throw ParseError(
                                "Invalid token (period can be used as either a separator between type of bound variable and expr, or a part of name in definition)",
                                tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                    }
                    break;
                }

                case ParseType::Term: {
                    if (stk->size() == 1) break;
                    load(2);
                    if (!stk->empty() && stash[0].ptype() == ParseType::Term) {
                        load(1);
                        // * T T    (e.g. % u (Term))
                        switch (stash[0].ptype()) {
                            case ParseType::Appl_WaitFirst: {
                                const auto term1 = stash[1].terms()[0];
                                const auto term2 = stash[2].terms()[0];
                                size_t end = stash[2].end();
                                stash[0].terms() = {appl(term1, term2)};
                                stash[0].change_ptype(ParseType::Term);
                                stash[0].end() = end;
                                flush(1);
                                reduced = true;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    flush(stash.size());

                    // debug(fstr(idx) << ", at T after * T T, stack = " << stack_dump(stk));

                    reduced |= contract_top_term(stk, false);
                    break;
                }

                case ParseType::AbstL_WaitVar:
                case ParseType::AbstP_WaitVar: {
                    atmosphere.push(stk->top().ptype());
                    break;
                }
                case ParseType::AbstL_WaitFirst:
                case ParseType::AbstL_WaitPeriod:
                case ParseType::AbstL_WaitSecond:
                case ParseType::AbstP_WaitFirst:
                case ParseType::AbstP_WaitPeriod:
                case ParseType::AbstP_WaitSecond:

                // tokens inactive at front
                case ParseType::Varname_Incomplete:
                case ParseType::Paren_Open:
                case ParseType::LineContinuator:
                case ParseType::Appl_WaitFirst:
                case ParseType::Appl_WaitSecond:
                case ParseType::_WaitTerm:
                case ParseType::Const_Name:
                case ParseType::Const_WaitTerm:
                case ParseType::Const_WaitCloseOrComma:
                case ParseType::Const_WaitTermOrClose:
                case ParseType::Arrow_Kind:
                case ParseType::Arrow_Implies:
                case ParseType::Arrow_Equiv: {
                    // need further fetch
                    // debug(fstr(idx) << ", need further fetch, before load");
                    load(2);
                    switch (stash[0].ptype()) {
                        case ParseType::Identifier_Str:
                        case ParseType::Varname_Incomplete: {
                            std::string name = stash[0].excerpt(tokens);
                            if (definitions.lookup_index(name) >= 0) {
                                throw ParseError(
                                    "The name of defined constant cannot be used as a variable name. Did you forget an argument list?",
                                    tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
                            }
                            stash[0].change_ptype(ParseType::Term);
                            stash[0].terms() = {variable(name)};
                            reduced = true;
                            break;
                        }
                        default:
                            // debug(fstr(idx) << ", need further fetch (stash: " << type_seq_to_str(stash));
                            break;
                    }
                    flush(2);
                    break;
                }
            }
        }

        // debug(fstr(idx) << " < " << end_of_token << ", reduction finished, stack: " << stack_dump(stk));
        // if (invalid_token_err) {
        //     debug(fstr(idx) << ", invalid token found");
        //     invalid_token_err->puterror();
        // }

        if (!stash.empty()) {
            std::string type_seq_str = type_seq_to_str(stash);
            throw ParseError(
                "Some tokens remain unflushed in stash (this is a bug)",
                tokens[idx],
                "Types of remaining tokens: " + type_seq_str,
                tokens[stash[0].begin()], tokens[stash.back().end() - 1]);
        }
        // if (stk->top().ptype() != ParseType::Term) continue;

        // front Term contraction
        // consecutive Term is possible with the length at most 2
        // 0. arrow contraction (T -> T -> ... -> T)
        // {
        //     load_arrow_seq(stash);
        //     if (stash.size() % 2 == 0) {
        //         throw ParseError(
        //             "A term-arrow sequence must start with a term",
        //             tokens[stash.front().begin()], tokens[stash.back().end() - 1],
        //             "The sequence starts from this arrow",
        //             tokens[stash[0].begin()], tokens[stash[0].end() - 1]);
        //     }
        //     elim_right_assoc(stash);
        //     flush(1);
        // }

        // finalization
        if (exhaust_token && invalid_token_err) {
            invalid_token_err->chain(ParseError(
                "Could not exhaust the given range",
                tokens[pos_init], tokens[end_of_token < tokens.size() ? end_of_token - 1 : pos_init]));
            throw *invalid_token_err;
        }
        if (idx + 1 == end_of_token || end_of_line || invalid_token_err) {
            if (invalid_token_err) stk->pop();
            reduced = true;
            while (reduced) {
                reduced = false;
                // debug(fstr(idx) << " < " << end_of_token << ", finalize 0/2, stack: " << stack_dump(stk));
                load_arrow_seq(stash);
                reduced |= elim_right_assoc(stash);
                flush(1);
                // debug(fstr(idx) << " < " << end_of_token << ", finalize 1/2, stack: " << stack_dump(stk));
                reduced |= contract_top_term(stk, true);
                // debug(fstr(idx) << " < " << end_of_token << ", finalize 2/2, stack: " << stack_dump(stk));
            }
            break;
        }
        ++idx;
    }

    // debug(fstr(stk->top().terms().size()));

    if (stk->top().terms().size() != 1) throw ParseError(
        "Could not obtain an expr by parsing this part",
        tokens[stk->top().begin()], tokens[stk->top().end() - 1]);

    // debug("return: " << stk->top().terms().front());
    return std::make_shared<ParseLambdaToken>(tokens[stk->top().begin()], tokens[idx = stk->top().end() - 1], stk->top().terms()[0]);
}

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx, const std::vector<std::shared_ptr<Context>>& flag_context, const Environment& definitions) {
    return parse_lambda(tokens, idx, tokens.size(), false, flag_context, definitions);
}

std::vector<std::shared_ptr<FileData>> raw_string_fds;

std::shared_ptr<Term> parse_lambda(const std::string& str, const std::vector<std::shared_ptr<Context>>& flag_context, const Environment& definitions) {
    size_t idx = 0;
    std::shared_ptr<FileData> fdp = std::make_shared<FileData>(FileData({str}, "[from raw string]"));
    raw_string_fds.push_back(fdp);
    auto tokens = tokenize(*fdp);
    return parse_lambda(tokens, idx, tokens.size(), true, flag_context, definitions)->term();
}
std::shared_ptr<Term> parse_lambda(const std::string& str, const Environment& definitions) {
    std::vector<std::shared_ptr<Context>> fc;
    return parse_lambda(str, fc, definitions);
}
std::shared_ptr<Term> parse_lambda(const std::string& str) {
    Environment env;
    return parse_lambda(str, env);
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
                        // expr = parse_lambda_old(tokens, idx, std::make_shared<std::vector<std::shared_ptr<Context>>>(flag_context));
                        expr = parse_lambda(tokens, idx, flag_context, env);
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
