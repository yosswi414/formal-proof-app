#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "common.hpp"

/*
#####  definitions  #####
note: [*] denotes a list of *
      <*> denotes a tuple
      M: N denotes that M is N
Book            [Judgement]
Judgement       <Environment, Context, Proof: ε, Prop: ε>
Environment     [Definition]
Definition      <Context, Constant, TypedTerm>
              | <Context, Constant, Axiom: ε>
Context         [<V, Type: ε>]
TypedTerm       <Term: ε, Type: ε>
ε               V | Star | Square | εε
              | λV:ε.ε | ΠV:ε.ε | Constant
Constant        <Name, [Type: ε]>
V               [a-zA-Z]
Name            [a-zA-Z][a-zA-Z0-9_-]+
 */

enum class Kind {
    Star,
    Square,
    Variable,
    Application,
    AbstLambda,
    AbstPi,
    Constant,
    Test01,
    Test02,
    Test03,
};

std::string to_string(const Kind& k) {
    switch (k) {
        case Kind::Star:
            return "Kind::Star";
        case Kind::Square:
            return "Kind::Square";
        case Kind::Variable:
            return "Kind::Variable";
        case Kind::Application:
            return "Kind::Application";
        case Kind::AbstLambda:
            return "Kind::AbstLambda";
        case Kind::AbstPi:
            return "Kind::AbstPi";
        case Kind::Constant:
            return "Kind::Constant";
        case Kind::Test01:
            return "Kind::Test01";
        case Kind::Test02:
            return "Kind::Test02";
        case Kind::Test03:
            return "Kind::Test03";
        default:
            return "[to_string(const Kind&): TBI]";
    }
}

std::ostream& operator<<(std::ostream& os, const Kind& k) {
    return os << to_string(k);
}

template <typename T>
using has_string_t = decltype(std::declval<T>().string());

template <typename PtrType, typename std::enable_if_t<
                                std::is_pointer<PtrType>::value || std::is_same<PtrType, std::unique_ptr<typename PtrType::element_type>>::value || std::is_same<PtrType, std::shared_ptr<typename PtrType::element_type>>::value, int> = 0>
std::ostream& operator<<(std::ostream& os, const PtrType& ptr) {
    return os << ptr->string();
}

template <class T, typename = std::enable_if_t<has_string_t<T>::value, void>>
std::ostream& operator<<(std::ostream& os, const T& x) {
    return os << x.string();
}

class Term {
  public:
    Term() = delete;
    virtual ~Term() = default;
    Kind kind() const { return _kind; }
    virtual std::string string() const = 0;
    virtual std::string repr() const { return string(); }

  protected:
    Term(const Kind& k) : _kind(k) {}

  private:
    Kind _kind;
};

class Star : public Term {
  public:
    Star() : Term(Kind::Star) {}
    std::string string() const override { return "*"; }
};

const std::string SYMBOL_SQUARE = (OnlyAscii ? "@" : "□");

class Square : public Term {
  public:
    Square() : Term(Kind::Square) {}
    std::string string() const override { return SYMBOL_SQUARE; }
    std::string repr() const override { return "@"; }
};

class Variable : public Term {
  public:
    Variable(char ch) : Term(Kind::Variable), _var_name(ch) {}
    std::string string() const override { return std::string(1, _var_name); }

  private:
    char _var_name;
};

class Application : public Term {
  public:
    Application(std::shared_ptr<Term> m, std::shared_ptr<Term> n) : Term(Kind::Application), _M(m), _N(n) {}

    const std::shared_ptr<Term>& M() const { return _M; }
    const std::shared_ptr<Term>& N() const { return _N; }

    std::string string() const override {
        return std::string("%(") + _M->string() + ")(" + _N->string() + ")";
    }
    std::string repr() const override {
        return std::string("%(") + _M->repr() + ")(" + _N->repr() + ")";
    }

  private:
    std::shared_ptr<Term> _M, _N;
};

template <typename T, std::enable_if_t<std::is_base_of<Term, T>::value, bool> = true>
class Typed {
  public:
    Typed(std::shared_ptr<T> val, std::shared_ptr<Term> t) : _value(val), _type(t) {}
    // ~Typed() { std::cerr << "~Typed(" << _value << " : " << _type << ")" << std::endl; }

    const std::shared_ptr<T>& value() const { return _value; }
    const std::shared_ptr<Term>& type() const { return _type; }

    std::string string() const {
        return _value->string() + ":" + _type->string();
    }

  private:
    std::shared_ptr<T> _value;
    std::shared_ptr<Term> _type;
};

const std::string SYMBOL_LAMBDA = (OnlyAscii ? "$" : "λ");

class AbstLambda : public Term {
  public:
    AbstLambda(std::shared_ptr<Typed<Variable>> v, std::shared_ptr<Term> e) : Term(Kind::AbstLambda), _var(v), _expr(e) {}

    const std::shared_ptr<Typed<Variable>>& var() const { return _var; }
    const std::shared_ptr<Term>& expr() const { return _expr; }

    std::string string() const override {
        return SYMBOL_LAMBDA + _var->string() + ".(" + _expr->string() + ")";
    }
    std::string repr() const override {
        return "$" + _var->value()->repr() + ":(" + _var->type()->repr() + ").(" + _expr->repr() + ")";
    }

  private:
    std::shared_ptr<Typed<Variable>> _var;
    std::shared_ptr<Term> _expr;
};

const std::string SYMBOL_PI = (OnlyAscii ? "?" : "Π");

class AbstPi : public Term {
  public:
    AbstPi(std::shared_ptr<Typed<Variable>> v, std::shared_ptr<Term> e) : Term(Kind::AbstPi), _var(v), _expr(e) {}

    const std::shared_ptr<Typed<Variable>>& var() const { return _var; }
    const std::shared_ptr<Term>& expr() const { return _expr; }

    std::string string() const override {
        return SYMBOL_PI + _var->string() + ".(" + _expr->string() + ")";
    }
    std::string repr() const override {
        return "?" + _var->value()->repr() + ":(" + _var->type()->repr() + ").(" + _expr->repr() + ")";
    }

  private:
    std::shared_ptr<Typed<Variable>> _var;
    std::shared_ptr<Term> _expr;
};

class Constant : public Term {
  public:
    Constant(const std::string& name, std::vector<std::shared_ptr<Term>> list) : Term(Kind::Constant), _name(name), _types(list) {}
    template <class... Ts>
    Constant(const std::string& name, Ts... ptrs) : Term(Kind::Constant), _name(name), _types{ptrs...} {}

    const std::vector<std::shared_ptr<Term>>& types() const { return _types; }

    std::string string() const override {
        std::string res(_name);
        res += "[";
        if (_types.size() > 0) res += _types[0]->string();
        for (size_t i = 1; i < _types.size(); ++i) res += ", " + _types[i]->string();
        res += "]";
        return res;
    }
    std::string repr() const override {
        std::string res(_name);
        res += "[";
        if (_types.size() > 0) res += "(" + _types[0]->repr() + ")";
        for (size_t i = 1; i < _types.size(); ++i) res += ", (" + _types[i]->repr() + ")";
        res += "]";
        return res;
    }
    const std::string& name() const { return _name; }

  private:
    std::string _name;
    std::vector<std::shared_ptr<Term>> _types;
};

template <Kind kind>
bool isTermA(const std::shared_ptr<Term> ptr) { return ptr->kind() == kind; }

const std::string SYMBOL_EMPTY = (OnlyAscii ? "{}" : "∅");
const std::string HEADER_CONTEXT = (OnlyAscii ? "Context" : "Γ");

class Context {
  public:
    Context() : _context{} {}
    Context(const std::vector<std::shared_ptr<Typed<Variable>>>& list) : _context(list) {}
    template <class... Ts>
    Context(Ts... vals) : _context{vals...} {}

    std::string string() const {
        std::string res("");
        if (_context.size() == 0) return SYMBOL_EMPTY;
        res += HEADER_CONTEXT + "{";
        if (_context.size() > 0) res += _context[0]->string();
        for (size_t i = 1; i < _context.size(); ++i) res += ", " + _context[i]->string();
        res += "}";
        return res;
    }
    std::string repr() const {
        std::string res("");
        res += std::to_string(_context.size()) + "\n";
        for (auto& ptr : _context) res += ptr->value()->repr() + "\n" + ptr->type()->repr() + "\n";
        return res;
    }

  private:
    std::vector<std::shared_ptr<Typed<Variable>>> _context;
};

const std::string DEFINITION_SEPARATOR = (OnlyAscii ? "|>" : "▷");
const std::string EMPTY_DEFINIENS = (OnlyAscii ? "#" : "⫫");

class Definition {
  public:
    Definition(std::shared_ptr<Context> context,
               std::shared_ptr<Constant> constant,
               std::shared_ptr<Term> prop) : _is_prim(true),
                                             _context(context),
                                             _definiendum(constant),
                                             _definiens(nullptr),
                                             _type(prop) {}

    Definition(std::shared_ptr<Context> context,
               std::shared_ptr<Constant> constant,
               std::shared_ptr<Term> proof,
               std::shared_ptr<Term> prop) : _is_prim(false),
                                             _context(context),
                                             _definiendum(constant),
                                             _definiens(proof),
                                             _type(prop) {}

    std::string string() const {
        std::string res;
        res = (_is_prim ? "Def-prim< " : "Def< ");
        res += _context->string();
        res += " " + DEFINITION_SEPARATOR + " " + _definiendum->string();
        res += " := " + (_is_prim ? EMPTY_DEFINIENS : _definiens->string());
        res += " : " + _type->string();
        res += " >";
        return res;
    }
    std::string repr() const {
        std::string res;
        res = "def2\n";
        res += _context->repr();
        res += _definiendum->name() + "\n";
        res += (_is_prim ? "#" : _definiens->repr()) + "\n";
        res += _type->repr() + "\n";
        res += "edef2\n";
        return res;
    }

    bool is_prim() const { return _is_prim; }
    const std::shared_ptr<Context>& context() const { return _context; }
    const std::shared_ptr<Constant>& definiendum() const { return _definiendum; }
    const std::shared_ptr<Term>& definiens() const { return _definiens; }
    const std::shared_ptr<Term>& type() const { return _type; }

  private:
    bool _is_prim;
    std::shared_ptr<Context> _context;
    std::shared_ptr<Constant> _definiendum;
    std::shared_ptr<Term> _definiens, _type;
};

const std::string HEADER_ENV = (OnlyAscii ? "Env" : "Δ");

class Environment {
  public:
    Environment() : _defs{} {}
    Environment(const std::vector<std::shared_ptr<Definition>>& list) : _defs(list) {}
    template <class... Ts>
    Environment(Ts... vals) : _defs{vals...} {}

    std::string string(bool inSingleLine = true, size_t indentSize = 0) const {
        std::string res = "";
        std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ?
        "" : "\t"), eol(inSingleLine ? " " : "\n");
        if (_defs.size() == 0) return indent_ex + SYMBOL_EMPTY;
        res += indent_ex + HEADER_ENV + "{{" + eol;
        if (_defs.size() > 0) res += indent_ex + indent_in + _defs[0]->string();
        for (size_t i = 1; i < _defs.size(); ++i) res += "," + eol + indent_ex + indent_in + _defs[i]->string();
        res += eol + indent_ex + "}}";
        return res;
    }

    std::string repr() const {
        std::string res = "";
        for (auto& ptr : _defs) res += ptr->repr() + "\n";
        res += "END\n";
        return res;
    }

    const std::vector<std::shared_ptr<Definition>>& defs() const { return _defs; }

  private:
    std::vector<std::shared_ptr<Definition>> _defs;
};

const std::string TURNSTILE = (OnlyAscii ? "|-" : "⊢");

class Judgement {
  public:
    Judgement(std::shared_ptr<Environment> defs,
              std::shared_ptr<Context> context,
              std::shared_ptr<Term> proof,
              std::shared_ptr<Term> prop) : _defs(defs), _context(context), _term(proof), _type(prop) {}
    std::string string(bool inSingleLine = true, size_t indentSize = 0) const {
        std::string res("");
        std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
        res += indent_ex + "Judge<<" + eol;
        res += _defs->string(inSingleLine, indentSize + (inSingleLine ? 0 : 1));
        res += " ;" + eol + indent_ex + indent_in + _context->string();
        res += " " + TURNSTILE + " " + _term->string();
        res += " : " + _type->string();
        res += eol + indent_ex + ">>";
        return res;
    }

    const std::shared_ptr<Environment>& defs() const { return _defs; }
    const std::shared_ptr<Context>& context() const { return _context; }
    const std::shared_ptr<Term>& term() const { return _term; }
    const std::shared_ptr<Term>& type() const { return _type; }

  private:
    std::shared_ptr<Environment> _defs;
    std::shared_ptr<Context> _context;
    std::shared_ptr<Term> _term, _type;
};

class Book {
  public:
    Book(): _judges{} {}
    Book(const std::vector<std::shared_ptr<Judgement>>& list) : _judges(list) {}
    template <class... Ts>
    Book(Ts... vals) : _judges{vals...} {}

    // inference rules
    void sort() {
        _judges.emplace_back(std::make_shared<Judgement>(
            std::make_shared<Environment>(),
            std::make_shared<Context>(),
            std::make_shared<Star>(),
            std::make_shared<Square>()));
    }
    void var(int m, char x) { unused(m, x); }
    void weak(int m, int n, char x) { unused(m, n, x); }
    void form(int m, int n) { unused(m, n); }

    std::string string() const {
        std::string res("Book[[");
        bool singleLine = true;
        int indentSize = 0;
        if (_judges.size() > 0) res += "\n" + _judges[0]->string(singleLine, indentSize);
        for (size_t i = 1; i < _judges.size(); ++i) res += ",\n" + _judges[i]->string(singleLine, indentSize);
        res += "\n]]";
        return res;
    }
    std::string repr() const { return "to be impl'ed"; }

  private:
    std::vector<std::shared_ptr<Judgement>> _judges;
};
