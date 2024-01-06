#include "lambda.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>

#include "old_parser.hpp"

/* [TODO]
 * - [done] tokenizer ("$x:(%(y)(z)).(*)" -> ['$'Lambda, 'x'Var, ':'Colon, '('LPar, '%'Appl, ...])
 * - parser ([...] -> AbstLambda)
 *      - use Location or some sort
 * - [done] let Location have original lines reference (too tedious to tell reference every time)
 * - (easy) for backward compatibility, convert def_file with new notation to the one written in conventional notation
 *      - e.g.) "$x:A.B" -> "$x:(A).(B)"
 *              "A:*" -> "A\n*"
 *              "implies := ?x:A.B : *" -> "implies\n?x:(A).(B)\n*"
 * [IDEA]
 * - rename: AbstLambda -> Lambda, AbstPi -> Pi
 */

void test_term() {
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
}

void test_derived() {
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
}

void test_old_parse() {
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

enum class TokenType {
    Unclassified,
    NewLine,  // '\n', "\r\n",
    Number,
    Variable,
    ConstName,
    ConstLeft,
    ConstRight,
    Comma,
    Colon,
    Backslash,
    Period,
    ParenLeft,
    ParenRight,
    Lambda,
    Pi,
    Appl,
    Star,
    Square,
    DefinedBy,  // ":="
    DefBegin,   // "def2"
    DefEnd,     // "edef2"
    EndOfFile,  // "END"
    Unknown
};

std::string to_string(const TokenType& t) {
    switch (t) {
        case TokenType::Unclassified: return "TokenType::Unclassified";
        case TokenType::NewLine: return "TokenType::NewLine";
        case TokenType::Number: return "TokenType::Number";
        case TokenType::Variable: return "TokenType::Variable";
        case TokenType::ConstName: return "TokenType::ConstName";
        case TokenType::ConstLeft: return "TokenType::ConstLeft";
        case TokenType::ConstRight: return "TokenType::ConstRight";
        case TokenType::Comma: return "TokenType::Comma";
        case TokenType::Colon: return "TokenType::Colon";
        case TokenType::Backslash: return "TokenType::Backslash";
        case TokenType::Period: return "TokenType::Period";
        case TokenType::ParenLeft: return "TokenType::ParenLeft";
        case TokenType::ParenRight: return "TokenType::ParenRight";
        case TokenType::Lambda: return "TokenType::Lambda";
        case TokenType::Pi: return "TokenType::Pi";
        case TokenType::Appl: return "TokenType::Appl";
        case TokenType::Star: return "TokenType::Star";
        case TokenType::Square: return "TokenType::Square";
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
        case '[': return TokenType::ConstLeft;
        case ']': return TokenType::ConstRight;
        case '$': return TokenType::Lambda;
        case '?': return TokenType::Pi;
        case '@': return TokenType::Square;
        case '*': return TokenType::Star;
        case ':': return TokenType::Colon;
        case '.': return TokenType::Period;
        case ',': return TokenType::Comma;
        case '\\': return TokenType::Backslash;
        default: return TokenType::Unknown;
    }
}

std::ostream& operator<<(std::ostream& os, const TokenType& t) {
    os << to_string(t);
    return os;
}

class Token {
  public:
    Token(const TextData& lines,
          size_t lno,
          size_t pos,
          size_t len,
          TokenType type = TokenType::Unclassified)
        : _lines(lines),
          _lno(lno),
          _pos(pos),
          _len(len),
          _type(type) {}
    Token(const TextData& lines,
          size_t lno,
          TokenType type = TokenType::Unclassified)
        : Token(lines, lno, std::string::npos, 0, type) {}
    std::string string() const {
        if (_len == 0 || _lines[_lno].size() <= _pos) return "";
        return _lines[_lno].substr(_pos, _len);
    }
    TokenType type() const { return _type; }

  private:
    const TextData& _lines;
    const size_t _lno, _pos, _len;
    TokenType _type;
};

std::vector<Token> tokenize(const TextData& lines) {
    std::vector<Token> tokens;
    const std::string sym_const = "_-.";
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
            if (ch == ':'){
                if (pos + 1 < lines[lno].size() && lines[lno][pos + 1] == '='){
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
                else if (pos + 1 < eon) t = TokenType::ConstName;
                else t = TokenType::Variable;
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
            ++pos;
        }
        tokens.emplace_back(lines, lno, TokenType::NewLine);
    }
    return tokens;
}

int main() {
    TextData lines = read_file("src/def_file_test");

    auto tokens = tokenize(lines);

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cerr << "token " << i << ": [" << tokens[i].string() << "] (" << tokens[i].type() << ")" << std::endl;
    }

    return 0;
}