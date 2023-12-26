// TODO: {done} raise parse exceptions and handle them to specify which line has a syntax error
// TODO: allow abbreviation of parenthesis
//      e.g.) M[(A),(u),(?x:(A).(B))] -> M[A,u,?x:A.B]
//      e.g.) $x.A.($y.B.C)DE -> $x.A.$y.B.CDE <- $x.A.($y.B.CD)E

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

const bool READ_FROM_CMD = true;
std::string fname;

[[noreturn]] void error();

enum class LambdaType {
    Variable,
    Square,
    Star,
    Bottom,
    Application,
    AbstractionLambda,
    AbstractionPi,
    Constant,
    LambdaTerm,
    LambdaTwo,
    LambdaN,
    Lambda,
    NA
};

std::ostream& operator<<(std::ostream& os, const LambdaType& T) {
    switch (T) {
        case LambdaType::Variable:
            return os << "LambdaType::Variable";
        case LambdaType::Square:
            return os << "LambdaType::Square";
        case LambdaType::Star:
            return os << "LambdaType::Star";
        case LambdaType::Bottom:
            return os << "LambdaType::Bottom";

        case LambdaType::Application:
            return os << "LambdaType::Application";

        case LambdaType::AbstractionLambda:
            return os << "LambdaType::AbstractionLambda";

        case LambdaType::AbstractionPi:
            return os << "LambdaType::AbstractionPi";

        case LambdaType::Constant:
            return os << "LambdaType::Constant";

        case LambdaType::LambdaTerm:
            return os << "LambdaType::LambdaTerm";

        case LambdaType::LambdaTwo:
            return os << "LambdaType::LambdaTwo";

        case LambdaType::LambdaN:
            return os << "LambdaType::LambdaTwo";

        case LambdaType::Lambda:
            return os << "LambdaType::Lambda";

        default:
            std::cerr << "operator<<(LambdaType): unknown type: [" << (int)T << "]" << std::endl;
            error();
    }
}

// const char varNA = '?';

class Lambda;
Lambda* parse_lambda(std::string line);
Lambda* parse_lambda(std::string line, int ofs);

class Lambda {
  public:
    Lambda(LambdaType T = LambdaType::Lambda) : _parent(nullptr), _type(T) { register_ptr(this); }
    virtual ~Lambda() = default;
    Lambda* parent() const { return _parent; }

    LambdaType type() const { return _type; }

    friend std::ostream& operator<<(std::ostream& os, const Lambda& defs);

  protected:
    Lambda* _parent;
    LambdaType _type;

    static int dels;
    static std::vector<Lambda*> trash;
    friend void register_ptr(Lambda* ptr);
    friend void cleanup();
};

int Lambda::dels;
std::vector<Lambda*> Lambda::trash;

void register_ptr(Lambda* ptr) {
    Lambda::trash.emplace_back(ptr);
}

// for variable, square, star
class LambdaTerm : public Lambda {
  public:
    LambdaTerm(char name, LambdaType T = LambdaType::LambdaTerm) : Lambda(T), _var_name(name) {}
    const char& var_name() const {
        // assert(_var_name != varNA);
        return _var_name;
    }
    char& var_name() {
        // assert(_var_name != varNA);
        return _var_name;
    }

  protected:
    char _var_name;
};

// for application, abstraction(lambda, pi)
class LambdaTwo : public Lambda {
  public:
    LambdaTwo(Lambda* l, Lambda* r, LambdaType T = LambdaType::LambdaTwo) : Lambda(T) {
        _left = l;
        _right = r;
    }
    LambdaTwo(const std::string& l, const std::string& r, LambdaType T = LambdaType::LambdaTwo) : LambdaTwo(parse_lambda(l), parse_lambda(r), T) {}
    const Lambda* left() const {
        assert(_left != nullptr);
        return _left;
    }

    const Lambda* right() const {
        assert(_right != nullptr);
        return _right;
    }
    Lambda*& left() {
        assert(_left != nullptr);
        return _left;
    }
    Lambda*& right() {
        assert(_right != nullptr);
        return _right;
    }

  protected:
    Lambda *_left, *_right;
};

// for constant
class LambdaN : public Lambda {
  public:
    LambdaN(const std::vector<Lambda*>& list, LambdaType T = LambdaType::LambdaN) : Lambda(T), _lambda_list(list) {}
    const std::vector<Lambda*>& lambda_list() const { return _lambda_list; }
    std::vector<Lambda*>& lambda_list() { return _lambda_list; }

  protected:
    std::vector<Lambda*> _lambda_list;
};

// LambdaTerm extension
class Variable : public LambdaTerm {
  public:
    Variable(char ch) : LambdaTerm(ch, LT) { assert(isalpha(ch)); }

  protected:
    static const LambdaType LT = LambdaType::Variable;
};
class Square : public LambdaTerm {
  public:
    Square() : LambdaTerm('@', LT) {}

  protected:
    static const LambdaType LT = LambdaType::Square;
};
class Star : public LambdaTerm {
  public:
    Star() : LambdaTerm('*', LT) {}

  protected:
    static const LambdaType LT = LambdaType::Star;
};
class Bottom : public LambdaTerm {
  public:
    Bottom() : LambdaTerm('#', LT) {}

  protected:
    static const LambdaType LT = LambdaType::Bottom;
};

// LambdaTwo extension
class Application : public LambdaTwo {
  public:
    Application(const std::string& l, const std::string& r) : LambdaTwo(l, r, LT) {}
    Application(Lambda* l, Lambda* r) : LambdaTwo(l, r, LT) {}

  protected:
    static const LambdaType LT = LambdaType::Application;
};
class AbstractionLambda : public LambdaTwo {
  public:
    AbstractionLambda(char var, const std::string& l, const std::string& r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}
    AbstractionLambda(char var, Lambda* l, Lambda* r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}
    AbstractionLambda(Variable var, const std::string& l, const std::string& r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}
    AbstractionLambda(Variable var, Lambda* l, Lambda* r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}

    Variable* bind() const { return _bind; }

  protected:
    static const LambdaType LT = LambdaType::AbstractionLambda;
    Variable* _bind;
};
class AbstractionPi : public LambdaTwo {
  public:
    AbstractionPi(char var, const std::string& l, const std::string& r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}
    AbstractionPi(char var, Lambda* l, Lambda* r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}
    AbstractionPi(Variable var, const std::string& l, const std::string& r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}
    AbstractionPi(Variable var, Lambda* l, Lambda* r) : LambdaTwo(l, r, LT), _bind(new Variable(var)) {}

    Variable* bind() const { return _bind; }

  protected:
    static const LambdaType LT = LambdaType::AbstractionPi;
    Variable* _bind;
};

// LambdaN extension
class Constant : public LambdaN {
  public:
    Constant(std::string name) : LambdaN({}, LT), _name(name) {}
    Constant(std::string name, const std::vector<Lambda*>& list) : LambdaN(list, LT), _name(name) {}
    std::string& name() { return _name; }
    const std::string& name() const { return _name; }

  protected:
    static const LambdaType LT = LambdaType::Constant;
    std::string _name;
};

template <typename Func>
size_t nextchar(size_t start, size_t end, const std::string& str, Func pred, int gnd = 0) {
    int depth = 0;
    for (size_t pos = start; pos < end; ++pos) {
        char ch = str[pos];
        if (pred(ch) && depth == gnd) return pos;
        else if (ch == '(') ++depth;
        else if (ch == ')') --depth;
        assert(depth >= 0);
    }
    return end;
}

size_t nextchar(size_t start, size_t end, const std::string& str, const char target) {
    return nextchar(
        start, end, str, [target](char ch) { return ch == target; }, target == ')' ? 1 : 0);
}

template <typename Func>
size_t nextchar(size_t start, const std::string& str, Func pred) {
    return nextchar(start, str.size(), str, pred);
}

size_t nextchar(size_t start, const std::string& str, const char target) {
    return nextchar(start, str.size(), str, target);
}

class InvalidSyntaxLambdaException {
  public:
    InvalidSyntaxLambdaException() = default;
    InvalidSyntaxLambdaException(const std::string& msg, int pos = -1) : message(msg), position(pos) {}
    std::string message;
    int position;
};

class InvalidSyntaxDefException {
  public:
    InvalidSyntaxDefException() = default;
    InvalidSyntaxDefException(const std::string& msg, int pos = -1) : message("def2: " + msg), line(pos), islambda(false) {}
    InvalidSyntaxDefException(const InvalidSyntaxLambdaException& e, int pos = -1) : line(pos), islambda(true), le(e) {}
    std::string message;
    int line;
    bool islambda;
    InvalidSyntaxLambdaException le;
};

class InvalidSyntaxFileException {
  public:
    InvalidSyntaxFileException() = default;
    InvalidSyntaxFileException(const std::string& msg, int pos = -1, int pos2 = -1) : message("file: " + msg), line(pos), line2(pos2), isdef(false) {}
    InvalidSyntaxFileException(const InvalidSyntaxDefException& e, int pos = -1, int pos2 = -1) : line(pos), line2(pos2), isdef(true), de(e) {}
    std::string message;
    int line, line2;
    bool isdef;
    InvalidSyntaxDefException de;
};

Lambda* parse_lambda(std::string line, int ofs) {
    int i = 0;
    char ch = line[i];
    if (isalpha(ch)) {  // constant, variable
        int j = i;
        while (j + 1 < (int)line.size() && [](char c) { return isalnum(c) || c == '_' || c == '-'; }(line[j + 1])) ++j;
        if (i < j) {  // constant
            std::string const_name = line.substr(i, j - i + 1);
            i = j + 1;
            if (line[i] != '[') throw InvalidSyntaxLambdaException("lambda (Constant): '[' not found after \"" + const_name + "\"", i + ofs);
            j = nextchar(++i, line, ']');
            if (line[j] != ']') throw InvalidSyntaxLambdaException("lambda (Constant): ']' not found after \"" + const_name + "\"", j + ofs);
            Constant* expr = new Constant(const_name);
            for (int k = i + 1; i < j; i = k + 1) {
                k = nextchar(i, j, line, ')') + 1;
                if (line[k] == ',' || line[k] == ']') expr->lambda_list().emplace_back(parse_lambda(line.substr(i, k - i), i + ofs));
                else throw InvalidSyntaxLambdaException("lambda (Constant): invalid comma separation during parsing arguments of \"" + const_name + "\"", k + ofs);
            }
            return expr;
        } else {  // variable
            if ((int)line.size() != 1) throw InvalidSyntaxLambdaException("lambda (Variable): invalid string \"" + line.substr(1) + "\" following after '" + ch + "'");
            return new Variable(ch);
        }
    }
    // ch is not an alphabet
    int j, i2, j2;
    char var;
    switch (ch) {
        case '*':  // star
            if (line.size() != 1) throw InvalidSyntaxLambdaException("lambda (Star): invalid string \"" + line.substr(1) + "\" following after '" + ch + "'");
            return new Star();
        case '@':  // square
            if (line.size() != 1) throw InvalidSyntaxLambdaException("lambda (Square): invalid string \"" + line.substr(1) + "\" following after '" + ch + "'");
            return new Square();
        case '#':  // bottom (_|_)
            if (line.size() != 1) throw InvalidSyntaxLambdaException("lambda (Bottom): invalid string \"" + line.substr(1) + "\" following after '" + ch + "'");
            return new Bottom();
        case '$':  // lambda
        case '?':  // pi
            var = line[++i];
            if (line[++i] != ':') throw InvalidSyntaxLambdaException("lambda (Abstraction): colon must follow after binding variable", i + ofs);
            if (line[++i] == '.') throw InvalidSyntaxLambdaException("lambda (Abstraction): first lambda cannot be empty", i + ofs);
            j = nextchar(i, line, '.');
            // std::cerr << "[debug: $?] line = " << line << " | i, j = " << i << ", " << j << std::endl;
            if (line[j] != '.') throw InvalidSyntaxLambdaException("lambda (Abstraction): two lambdas must be separated with period", j + ofs);
            i2 = j + 1;
            if (line[i2] != '(') throw InvalidSyntaxLambdaException("lambda (Abstraction): second lambda needs to be wrapped by parenthesis", i2 + ofs);
            j2 = nextchar(i2, line, ')');
            if (line[j2] != ')') throw InvalidSyntaxLambdaException("lambda (Abstraction): closing parenthesis ')' of second lambda not found", i2 + ofs);
            if (j2 + 1 != (int)line.size()) throw InvalidSyntaxLambdaException("lambda (Abstraction): invalid string \"" + line.substr(j2 + 1) + "\" following after second lambda", j2 + 1 + ofs);
            if (ch == '$') return new AbstractionLambda(var, line.substr(i, j - i), line.substr(i2, j2 - i2 + 1));
            return new AbstractionPi(var, line.substr(i, j - i), line.substr(i2, j2 - i2 + 1));
        case '%':  // application
            if (line[++i] != '(') throw InvalidSyntaxLambdaException("lambda (Application): lambda must be wrapped by parenthesis", i + ofs);
            j = nextchar(i, line, ')');
            if (line[j] != ')') throw InvalidSyntaxLambdaException("lambda (Application): closing parenthesis ')' of first lambda not found", i + ofs);
            if (line[i2 = j + 1] != '(') throw InvalidSyntaxLambdaException("lambda (Application): second opening parenthesis should come right after the first closing parenthesis", i2 + ofs);
            j2 = nextchar(i2, line, ')');
            if (line[j2] != ')') throw InvalidSyntaxLambdaException("lambda (Application): closing parenthesis ')' of second lambda not found", i2 + ofs);
            if (j2 + 1 != (int)line.size()) throw InvalidSyntaxLambdaException("lambda (Application): invalid string \"" + line.substr(j2 + 1) + "\" following after second lambda", j2 + 1 + ofs);
            return new Application(line.substr(i, j - i + 1), line.substr(i2, j2 - i2 + 1));
        case '(':
            if (line.back() != ')') throw InvalidSyntaxLambdaException("lambda (parenthesis): not closing or followed by invalid tokens after ')'");
            return parse_lambda(line.substr(1, line.size() - 2), 1 + ofs);
        default:
            throw InvalidSyntaxLambdaException(std::string("lambda (symbol): unknown symbol '") + ch + "'");
    }
}

Lambda* parse_lambda(std::string line) { return parse_lambda(line, 0); }

class Def2 {
  public:
    std::vector<std::pair<Variable*, Lambda*>> args;
    std::string name;
    Lambda *left, *right;

    Def2(const std::vector<std::string>& lines, int start, int end) {
        bool head = false;
        bool argc = false;
        bool bname = false;
        bool foot = false;
        int n = -1;
        for (int i = start; i < end; ++i) {
            if (lines[i].size() == 0) continue;
            if (foot) throw InvalidSyntaxDefException("content \"" + lines[i] + "\" found after 'edef2'", i);  // fails with content after "edef2"
            if (lines[i] == "def2") {
                head = true;
                continue;
            }
            if (lines[i] == "edef2") {
                if (!head) throw InvalidSyntaxDefException("'edef2' found before 'def2'", i);
                foot = true;
                continue;
            }
            if (!head) throw InvalidSyntaxDefException("content \"" + lines[i] + "\" found before 'def2'", i);  // fails with content before "def2"
            if (!argc) {
                n = std::stoi(lines[i]);
                argc = true;
                continue;
            }
            if (n > 0) {
                if (lines[i].size() != 1) throw InvalidSyntaxDefException("x_i should be a variable, defined as a single alphabet", i);
                args.emplace_back(new Variable(lines[i][0]), parse_lambda(lines[i + 1]));
                ++i;
                --n;
                continue;
            }
            assert(n == 0);
            if (!bname) {
                if (lines[i].size() <= 1) throw InvalidSyntaxDefException("definition name length must be > 1", i);
                name = lines[i];
                bname = true;
                continue;
            }
            if (!foot) {
                try {
                    left = parse_lambda(lines[i]);
                } catch (const InvalidSyntaxLambdaException& e) {
                    throw InvalidSyntaxDefException(e, i);
                    // std::cerr << "l." << i << ": " << e.message << std::endl;
                    // std::string lno = std::to_string(i) + ' ';
                    // std::cerr << lno << "| " << lines[i] << std::endl;
                    // if (e.position >= 0) std::cerr << std::string(lno.size(), ' ') << "| " << std::string(e.position, ' ') + '^' << std::endl;
                    // error();
                }
                ++i;
                try {
                    right = parse_lambda(lines[i]);
                } catch (const InvalidSyntaxLambdaException& e) {
                    throw InvalidSyntaxDefException(e, i);
                    // std::cerr << "l." << i << ": " << e.message << std::endl;
                    // std::string lno = std::to_string(i) + ' ';
                    // std::cerr << lno << "| " << lines[i] << std::endl;
                    // if (e.position >= 0) std::cerr << std::string(lno.size(), ' ') << "| " << std::string(e.position, ' ') + '^' << std::endl;
                    // error();
                }
                continue;
            }
            throw InvalidSyntaxDefException("redundant line between 2 argument lines and 'edef2'", i);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Def2& def);
};

std::ostream& operator<<(std::ostream& os, const Def2& def) {
    for (size_t i = 0; i < def.args.size(); ++i) {
        os << "\t" << *def.args[i].first << " : " << *def.args[i].second << std::endl;
    }
    os << "\t[name] " << def.name << std::endl;
    os << "\t[arg1] " << *def.left << std::endl;
    os << "\t[arg2] " << *def.right << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Lambda& expr) {
    // should be done in one line
    switch (expr.type()) {
        case LambdaType::Variable:
        case LambdaType::Square:
        case LambdaType::Star:
        case LambdaType::Bottom:
            os << dynamic_cast<const LambdaTerm&>(expr).var_name();
            return os;
        case LambdaType::Application: {
            const Application& obj = dynamic_cast<const Application&>(expr);
            os << "%(" << *obj.left() << ")(" << *obj.right() << ")";
            return os;
        }
        case LambdaType::AbstractionLambda: {
            const AbstractionLambda& obj = dynamic_cast<const AbstractionLambda&>(expr);
            os << "$" << *obj.bind() << ":(" << *obj.left() << ").(" << *obj.right() << ")";
            return os;
        }
        case LambdaType::AbstractionPi: {
            const AbstractionPi& obj = dynamic_cast<const AbstractionPi&>(expr);
            os << "?" << *obj.bind() << ":(" << *obj.left() << ").(" << *obj.right() << ")";
            return os;
        }
        case LambdaType::Constant: {
            const Constant& obj = dynamic_cast<const Constant&>(expr);
            os << obj.name() << "[";
            size_t n = obj.lambda_list().size();
            if (n > 0) os << *obj.lambda_list()[0];
            for (size_t i = 1; i < obj.lambda_list().size(); ++i) {
                os << ", " << *obj.lambda_list()[i];
            }
            os << "]";
            return os;
        }
        case LambdaType::LambdaTerm:
            os << "{term}(" << dynamic_cast<const LambdaTerm&>(expr).var_name() << ")";
            return os;
        case LambdaType::LambdaTwo: {
            const LambdaTwo& obj = dynamic_cast<const LambdaTwo&>(expr);
            os << "{2}"
               << "(" << *obj.left() << ")(" << *obj.right() << ")";
            return os;
        }
        case LambdaType::LambdaN: {
            const LambdaN& obj = dynamic_cast<const LambdaN&>(expr);
            size_t n = obj.lambda_list().size();
            os << "{N = " << n << "}";
            for (size_t i = 0; i < obj.lambda_list().size(); ++i) {
                os << "(" << *obj.lambda_list()[i] << ")";
            }
            return os;
        }
        case LambdaType::Lambda:
            os << "{lambda; type = " << expr.type() << "}";
            return os;
        default:
            std::cerr << "operator<<(Lambda): unknown type: [" << (int)expr.type() << "]" << std::endl;
            error();
    }
}

void parse(const std::vector<std::string>& lines, std::vector<Def2>& data) {
    int eof = 0;
    for (; eof < (int)lines.size() && lines[eof] != "END"; ++eof) {}
    if (lines[eof] != "END") throw InvalidSyntaxFileException("'END' not found");
    for (int i = 0, j = 0; j < eof; i = j) {
        bool def2 = false;
        while (lines[j] != "edef2" && j + 1 < eof) {
            if (lines[j] == "def2"){
                if (def2) throw InvalidSyntaxFileException("new 'def2' before closing previous 'def2'", j);
                def2 = true;
            }
            ++j;
        }
        if (lines[j] != "edef2") {
            if (def2) {
                throw InvalidSyntaxFileException("definition not closed with 'edef2'", i, j);
                // int numlen = std::to_string(j - 1).size();
                // std::cerr << "ll." << i + 1 << " - " << j - 1 << ": file: definition not closed with 'edef2'" << std::endl;
                // for (int k = i; k < j; ++k) {
                //     if (lines[k].size() == 0) continue;
                //     std::string num = std::to_string(k);
                //     std::cerr << std::string(numlen - num.size(), ' ') << num << " | " << lines[k] << std::endl;
                // }
                // error();
            }
            if (j + 1 == eof) {
                while (i < j) {
                    if (lines[i].size() != 0) throw InvalidSyntaxFileException("invalid line before 'END'", i);
                    ++i;
                }
                break;
            }

        }
        try{
        data.emplace_back(lines, i, ++j);
        }catch(const InvalidSyntaxDefException& e){
            throw InvalidSyntaxFileException(e, i, j);
        }
    }
}

class Definitions {
  public:
    std::vector<Def2> list;

    Definitions() {}
    Definitions(const std::vector<std::string>& lines) { parse(lines, list); }
    friend std::ostream& operator<<(std::ostream& os, const Definitions& defs);
};

std::ostream& operator<<(std::ostream& os, const Definitions& defs) {
    for (size_t i = 0; i < defs.list.size(); ++i) {
        os << "def [" << i << "]:" << std::endl;
        os << defs.list[i] << std::endl;
    }
    return os;
}

std::string& trim(std::string& str) {
    str.erase(std::remove_if(str.begin(), str.end(), [](char ch) -> bool {
                  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
              }),
              str.end());
    return str;
}

std::vector<std::string> read_def_file(const std::string& fname) {
    std::ifstream ifs(fname);
    std::vector<std::string> lines;
    std::string str;
    // bool bs = false;  // backslash
    while (std::getline(ifs, str)) {
        size_t len = 0;
        for (; len + 1 < str.size(); ++len) {
            if (str.substr(len, 2) == "//") break;
        }
        while (len < str.size() && str[len] != '/') ++len;
        std::string trimmed = str.substr(0, len);
        trim(trimmed);
        // size_t k = trimmed.size();
        // bool nextbs = trimmed.back() == '\\';
        // if (nextbs) trimmed = trimmed.substr(0, k - 1);
        // if (bs) lines.back() += trimmed;
        // else lines.emplace_back(trimmed);
        // bs = nextbs;
        lines.emplace_back(trimmed);
    }
    return lines;
}

void cleanup() {
    std::map<LambdaType, int> dict;
    for (Lambda* ptr : Lambda::trash) {
        ++dict[ptr->type()];
        switch (ptr->type()) {
            case LambdaType::Variable:
                delete (Variable*)ptr;
                break;
            case LambdaType::Square:
                delete (Square*)ptr;
                break;
            case LambdaType::Star:
                delete (Star*)ptr;
                break;
            case LambdaType::Bottom:
                delete (Bottom*)ptr;
                break;
            case LambdaType::Application:
                delete (Application*)ptr;
                break;
            case LambdaType::AbstractionLambda:
                delete (AbstractionLambda*)ptr;
                break;
            case LambdaType::AbstractionPi:
                delete (AbstractionPi*)ptr;
                break;
            case LambdaType::Constant:
                delete (Constant*)ptr;
                break;
            case LambdaType::LambdaTerm:
                delete (LambdaTerm*)ptr;
                break;
            case LambdaType::LambdaTwo:
                delete (LambdaTwo*)ptr;
                break;
            case LambdaType::LambdaN:
                delete (LambdaN*)ptr;
                break;
            case LambdaType::Lambda:
                delete (Lambda*)ptr;
                break;
            default:
                std::cerr << "cleanup: unknown type: [" << (int)ptr->type() << "]" << std::endl;
                error();
        }
    }

    // std::vector<Lambda*>().swap(trash);

    // std::cerr << "Variable      : " << dict[LambdaType::Variable] << std::endl;
    // std::cerr << "Square        : " << dict[LambdaType::Square] << std::endl;
    // std::cerr << "Star          : " << dict[LambdaType::Star] << std::endl;
    // std::cerr << "Application   : " << dict[LambdaType::Application] << std::endl;
    // std::cerr << "AbstLambda    : " << dict[LambdaType::AbstractionLambda] << std::endl;
    // std::cerr << "AbstPi        : " << dict[LambdaType::AbstractionPi] << std::endl;
    // std::cerr << "Constant      : " << dict[LambdaType::Constant] << std::endl;
    // std::cerr << "LambdaTerm    : " << dict[LambdaType::LambdaTerm] << std::endl;
    // std::cerr << "LambdaTwo     : " << dict[LambdaType::LambdaTwo] << std::endl;
    // std::cerr << "LambdaN       : " << dict[LambdaType::LambdaN] << std::endl;
    // std::cerr << "Lambda        : " << dict[LambdaType::Lambda] << std::endl;
}

[[noreturn]] void error() {
    std::cerr << std::endl
              << "Aborting." << std::endl;
    cleanup();
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (READ_FROM_CMD) {
        if (argc != 2) {
            std::cerr << "Usage: ./test_read def_file" << std::endl;
            exit(EXIT_FAILURE);
        }
        fname = argv[1];
    } else {
        fname = "def_file";
    }

    std::vector<std::string> lines = read_def_file(fname);
    Definitions defs;
    try {
        defs = Definitions(lines);
    } catch (const InvalidSyntaxFileException& e) {
        if(!e.isdef){
            int i = e.line, j = e.line2;
            // position
            if (j >= 0) std::cerr << fname << ":" << i + 1 << " - " << j << ": ";
            else if (i >= 0) std::cerr << fname << ":" << i + 1 << ": ";
            // message
            std::cerr << e.message << std::endl;
            // show content
            if(j>=0){
                int numlen = std::to_string(j).size();
                // std::cerr << "ll." << i + 1 << " - " << j - 1 << ": file: definition not closed with 'edef2'" << std::endl;
                for (int k = i; k < j; ++k) {
                    if (lines[k].size() == 0) continue;
                    std::string num = std::to_string(k + 1);
                    std::cerr << std::string(numlen - num.size(), ' ') << num << " | " << lines[k] << std::endl;
                }
            } else if (i >= 0) std::cerr << i + 1 << " | " << lines[i] << std::endl;
            error();
        }
        auto& de = e.de;
        if (!de.islambda) {
            int i = de.line;
            if (i >= 0) std::cerr << fname << ":" << i + 1 << ": ";
            std::cerr << de.message << std::endl;
            if (i >= 0) std::cerr << i + 1 << " | " << lines[i] << std::endl;
            error();
        }
        auto& le = e.de.le;
        int i = de.line;
        std::cerr << fname << ":" << i + 1;
        if (le.position >= 0) std::cerr << ":" << le.position + 1;
        std::cerr << ": ";
        std::cerr << le.message << std::endl;
        std::string num = std::to_string(i + 1);
        std::cerr << num << " | " << lines[i] << std::endl;
        if(le.position >= 0) std::cerr << std::string(num.size(), ' ') << " | " << std::string(le.position, ' ') << '^' << std::endl;
    }

    std::cout << defs << std::endl;

    std::cout << "[end of definitions]" << std::endl;
    cleanup();
}
