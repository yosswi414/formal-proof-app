#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "common.hpp"
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
    DefinedBy,  // ":="
    DefBegin,   // "def2"
    DefEnd,     // "edef2"
    EndOfFile,  // "END"
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
    std::string pos_info_str() const {
        std::string res(_data.name());
        if (res.size() > 0) res += ":";
        res += std::to_string(lno() + 1);
        res += ":" + std::to_string(pos() + 1);
        if (_len > 1) res += "-" + std::to_string(std::min(pos() + len(), _data[_lno].size()) + 1);
        return res;
    }

  private:
    const FileData& _data;
    const size_t _lno, _pos, _len;
    TokenType _type;
};

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
          _note(std::make_shared<BaseError>("Note", data...)) {}
    template <typename... Ts>
    BaseError(const std::string& errtype, const std::string& msg, const Token& token_l, const Token& token_r, Ts... data)
        : _errtype(errtype),
          _msg(msg),
          _token(token_l),
          _token2(token_r),
          _note(std::make_shared<BaseError>("Note", data...)) {}

    void puterror(std::ostream& os = std::cerr) const {
        os << _token.pos_info_str() << ": " << _errtype << ": " << _msg << std::endl;
        if (_token.lno() == _token2.lno() && _token.pos() == _token2.pos()) {
            std::string line_no = std::to_string(_token.lno() + 1);
            os << line_no << " | " << _token.line() << std::endl;
            if (_token.len() > 0) {
                os << std::string(line_no.size(), ' ') << " | " << std::string(_token.pos(), ' ');
                os << "^" << std::string(_token.len() - 1, '~') << std::endl;
            }
        } else {
            size_t lno_begin = _token.lno(), lno_end = _token2.lno();
            size_t lno_str_len = std::to_string(lno_end + 1).size();
            os << std::string(lno_str_len, ' ') << " | " << std::string(_token.pos(), ' ') << "v" << std::string("~~~...").substr(0, _token.line().size() - 1 - _token.pos()) << std::endl;
            for (size_t lno_i = lno_begin; lno_i <= lno_end; ++lno_i) {
                std::string lno_i_str = std::to_string(lno_i + 1);
                os << std::string(lno_str_len - lno_i_str.size(), ' ') << lno_i << " | " << _token.filedata()[lno_i] << std::endl;
            }
            int end_pos = _token2.pos() + _token2.len() - 1;
            os << std::string(lno_str_len, ' ') << " | " << std::string(std::max(end_pos - 6, 0), ' ') << std::string("...~~~").substr(std::max(6 - end_pos, 0)) << "^" << std::endl;
        }
        if (_note) _note->puterror();
    }

    void bind(const BaseError& e) { _note = std::make_shared<BaseError>(e); }

  private:
    std::string _errtype, _msg;
    Token _token, _token2;
    std::shared_ptr<BaseError> _note;
};

#define DEFINE_ERROR(name)                                                                                                                         \
    class name : public BaseError {                                                                                                                \
      public:                                                                                                                                      \
        name(const std::string& msg, const Token& token) : BaseError(#name, msg, token) {}                                                         \
        name(const std::string& msg, const Token& token_l, const Token& token_r) : BaseError(#name, msg, token_l, token_r) {}                      \
        template <typename... Ts>                                                                                                                  \
        name(const std::string& msg, const Token& token, Ts... data) : BaseError(#name, msg, token, data...) {}                                    \
        template <typename... Ts>                                                                                                                  \
        name(const std::string& msg, const Token& token_l, const Token& token_r, Ts... data) : BaseError(#name, msg, token_l, token_r, data...) {} \
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

  private:
    Token _token_begin, _token_end;
    std::shared_ptr<Term> _term;
};

std::shared_ptr<ParseLambdaToken> parse_lambda(const std::vector<Token>& tokens, size_t& idx);

Environment parse(const std::vector<Token>& tokens);