#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "environment.hpp"
#include "lambda.hpp"

enum class TokenType {
    Unclassified,
    NewLine,  // '\n', "\r\n",
    Number,
    Character,
    String,
    SquareBracketLeft,
    SquareBracketRight,
    Comma,
    Colon,
    Semicolon,
    Backslash,
    Period,
    ParenLeft,
    ParenRight,
    CurlyBracketLeft,
    CurlyBracketRight,
    DollarSign,
    QuestionMark,
    Percent,
    Asterisk,
    AtSign,
    Hash,
    Underscore,
    Hyphen,
    Verticalbar,
    Plus,
    Spaces,                // ' ', '\t'
    Leftarrow,             // <-
    Rightarrow,            // ->
    Leftrightarrow,        // <->
    Leftdoublearrow,       // <=
    Rightdoublearrow,      // =>
    Leftrightdoublearrow,  // <=>
    DefinedBy,             // ":="
    DefBegin,              // "def2"
    DefEnd,                // "edef2"
    EndOfFile,             // "END"
    Unknown
};

std::string to_string(const TokenType& t);

TokenType sym2tokentype(char ch);

std::ostream& operator<<(std::ostream& os, const TokenType& t);

class Token {
  public:
    Token(const FileData& lines,
          size_t lno,
          size_t pos,
          size_t len,
          TokenType type = TokenType::Unclassified)
        : _data(lines),
          _lno(lno),
          _pos(pos),
          _len(len),
          _type(type) {}
    Token(const FileData& lines,
          size_t lno,
          TokenType type = TokenType::Unclassified)
        : Token(lines, lno, std::string::npos, 0, type) {}
    std::string string() const {
        if (_len == 0 || _data[_lno].size() <= _pos) return "";
        return _data[_lno].substr(_pos, _len);
    }
    const std::string& line() const { return _data[_lno]; }
    std::string filename() const { return _data.name(); }
    TokenType type() const { return _type; }
    size_t lno() const { return std::min(_lno, _data.size() - 1); }
    size_t pos() const { return std::min(_pos, _data[lno()].size()); }
    size_t len() const { return std::min(_len, _data[lno()].size()); }
    const FileData& filedata() const { return _data; }
    // std::string pos_info_str() const {
    //     std::string res(_data.name());
    //     if (res.size() > 0) res += ":";
    //     res += std::to_string(lno() + 1);
    //     res += ":" + std::to_string(pos() + 1);
    //     if (_len > 1) res += "-" + std::to_string(std::min(pos() + len(), _data[_lno].size()) + 1);
    //     return res;
    // }
    // friend std::string pos_info_str(const Token& t1, const Token& t2);

  private:
    const FileData& _data;
    const size_t _lno, _pos, _len;
    TokenType _type;
};

std::string pos_info_str(const Token& t1, const Token& t2);
std::string pos_info_str(const Token& t);

std::string to_string(const Token& t);

std::ostream& operator<<(std::ostream& os, const Token& t);

class BaseError {
  public:
    BaseError(const std::string& errtype, const std::string& msg, const Token& token)
        : _errtype(errtype),
          _msg(msg),
          _token(token),
          _token2(token),
          _note(nullptr) {}
    BaseError(const std::string& errtype, const std::string& msg, const Token& token_l, const Token& token_r)
        : _errtype(errtype),
          _msg(msg),
          _token(token_l),
          _token2(token_r),
          _note(nullptr) {}
    template <typename... Ts>
    BaseError(const std::string& errtype, const std::string& msg, const Token& token, Ts... data)
        : _errtype(errtype),
          _msg(msg),
          _token(token),
          _token2(token),
          _note(std::make_shared<BaseError>(BOLD(CYAN("Note")), data...)) {}
    template <typename... Ts>
    BaseError(const std::string& errtype, const std::string& msg, const Token& token_l, const Token& token_r, Ts... data)
        : _errtype(errtype),
          _msg(msg),
          _token(token_l),
          _token2(token_r),
          _note(std::make_shared<BaseError>(BOLD(CYAN("Note")), data...)) {}

    void puterror(std::ostream& os = std::cerr) const {
        os << pos_info_str(_token, _token2) << ": " << _errtype << ": " << _msg << std::endl;
        size_t lno1 = _token.lno();
        size_t lno2 = _token2.lno();
        size_t pos1 = _token.pos();
        size_t pos2 = _token2.pos();
        size_t len1 = _token.len();
        size_t len2 = _token2.len();

        if (lno1 == lno2) {
            std::string line_no = std::to_string(lno1 + 1);
            os << line_no << " | " << _token.line() << std::endl;
            if (len1 > 0) {
                size_t from, to, l;
                from = std::min(pos1, pos2);
                to = std::max(pos1 + len1, pos2 + len2);
                l = to - from;
                os << std::string(line_no.size(), ' ') << " | " << std::string(from, ' ');
                os << "^" << std::string(l - 1, '~') << std::endl;
            }
        } else {
            size_t lno_begin = lno1, lno_end = lno2;
            size_t lno_str_len = std::to_string(lno_end + 1).size();
            os << std::string(lno_str_len, ' ') << " | " << std::string(pos1, ' ') << "v" << std::string("~~~...").substr(0, _token.line().size() - 1 - pos1) << std::endl;
            for (size_t lno_i = lno_begin; lno_i <= lno_end; ++lno_i) {
                std::string lno_i_str = std::to_string(lno_i + 1);
                os << std::string(lno_str_len - lno_i_str.size(), ' ') << lno_i + 1 << " | " << _token.filedata()[lno_i] << std::endl;
            }
            int end_pos = pos2 + len2 - 1;
            os << std::string(lno_str_len, ' ') << " | " << std::string(std::max(end_pos - 6, 0), ' ') << std::string("...~~~").substr(std::max(6 - end_pos, 0)) << "^" << std::endl;
        }
        if (_note) _note->puterror();
        if (_next) _next->puterror();
    }

    void bind(const BaseError& e) { _note = std::make_shared<BaseError>(e); }
    void chain(const BaseError& e) { _next = std::make_shared<BaseError>(e); }

  private:
    std::string _errtype, _msg;
    Token _token, _token2;
    std::shared_ptr<BaseError> _note, _next;
};

#define DEFINE_ERROR(name)                                                                                                                                    \
    class name : public BaseError {                                                                                                                           \
      public:                                                                                                                                                 \
        name(const std::string& msg, const Token& token) : BaseError(BOLD(RED(#name)), msg, token) {}                                                         \
        name(const std::string& msg, const Token& token_l, const Token& token_r) : BaseError(BOLD(RED(#name)), msg, token_l, token_r) {}                      \
        template <typename... Ts>                                                                                                                             \
        name(const std::string& msg, const Token& token, Ts... data) : BaseError(BOLD(RED(#name)), msg, token, data...) {}                                    \
        template <typename... Ts>                                                                                                                             \
        name(const std::string& msg, const Token& token_l, const Token& token_r, Ts... data) : BaseError(BOLD(RED(#name)), msg, token_l, token_r, data...) {} \
    }

DEFINE_ERROR(TokenizeError);
DEFINE_ERROR(ParseError);
DEFINE_ERROR(ExprError);

#undef DEFINE_ERROR

std::vector<Token> tokenize(const FileData& lines);

class ParseLambdaToken {
  public:
    ParseLambdaToken(const Token& token, std::shared_ptr<Term> term = nullptr) : _token_begin(token), _token_end(token), _term(term) {}
    ParseLambdaToken(const Token& token_l, const Token& token_r, std::shared_ptr<Term> term = nullptr) : _token_begin(token_l), _token_end(token_r), _term(term) {}
    std::shared_ptr<Term> term() { return _term; }
    Token& begin() { return _token_begin; }
    Token& end() { return _token_end; }
    const std::shared_ptr<Term> term() const { return _term; }
    const Token& begin() const { return _token_begin; }
    const Token& end() const { return _token_end; }

  private:
    Token _token_begin, _token_end;
    std::shared_ptr<Term> _term;
};

std::shared_ptr<ParseLambdaToken> parse_lambda_new(const std::vector<Token>& tokens, size_t& idx, size_t end_of_token, bool exhaust_token, const std::vector<std::shared_ptr<Context>>& flag_context, const Environment& definitions);
std::shared_ptr<ParseLambdaToken> parse_lambda_new(const std::vector<Token>& tokens, size_t& idx, const std::vector<std::shared_ptr<Context>>& flag_context, const Environment& definitions);
std::shared_ptr<Term> parse_lambda_new(const std::string& str, const std::vector<std::shared_ptr<Context>>& flag_context, const Environment& definitions);
std::shared_ptr<Term> parse_lambda_new(const std::string& str, const Environment& definitions);
std::shared_ptr<Term> parse_lambda_new(const std::string& str);

std::shared_ptr<ParseLambdaToken> parse_lambda_old(const std::vector<Token>& tokens, size_t& idx, size_t end_of_token, bool exhaust_token, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context, bool no_chainer = false);
std::shared_ptr<ParseLambdaToken> parse_lambda_old(const std::vector<Token>& tokens, size_t& idx, size_t end_of_token, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context, bool no_chainer = false);
std::shared_ptr<ParseLambdaToken> parse_lambda_old(const std::vector<Token>& tokens, size_t& idx, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context, bool no_chainer = false);

std::shared_ptr<Term> parse_lambda_old(const std::string& str, const std::shared_ptr<std::vector<std::shared_ptr<Context>>>& flag_context = nullptr);

Environment parse_defs(const std::vector<Token>& tokens);
