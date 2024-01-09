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

std::string to_string(const Kind& k);

template <typename PtrType, typename std::enable_if_t<
                                std::is_pointer<PtrType>::value || std::is_same<PtrType, std::unique_ptr<typename PtrType::element_type>>::value || std::is_same<PtrType, std::shared_ptr<typename PtrType::element_type>>::value,
                                int> = 0>
std::ostream& operator<<(std::ostream& os, const PtrType& ptr) {
    return os << ptr->string();
}

std::ostream& operator<<(std::ostream& os, const Kind& k);

template <typename T, typename = std::void_t<decltype(std::declval<T>().string())>>
std::ostream& operator<<(std::ostream& os, const T& x) {
    os << x.string();
    return os;
}

class Term {
  public:
    Term() = delete;
    virtual ~Term() = default;
    Kind kind() const { return _kind; }
    virtual std::string string() const = 0;
    virtual std::string repr() const { return string(); }
    virtual std::string repr_new() const { return repr(); }

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
    const char& name() const { return _var_name; }
    char& name() { return _var_name; }

  private:
    char _var_name;
};

class Application : public Term {
  public:
    Application(std::shared_ptr<Term> m, std::shared_ptr<Term> n) : Term(Kind::Application), _M(m), _N(n) {}

    const std::shared_ptr<Term>& M() const { return _M; }
    const std::shared_ptr<Term>& N() const { return _N; }
    std::shared_ptr<Term>& M() { return _M; }
    std::shared_ptr<Term>& N() { return _N; }

    std::string string() const override {
        return std::string("%") + _M->string() + " " + _N->string();
    }
    std::string repr() const override {
        return std::string("%(") + _M->repr() + ")(" + _N->repr() + ")";
    }
    std::string repr_new() const override {
        return std::string("%") + _M->repr_new() + " " + _N->repr_new();
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
    std::shared_ptr<T>& value() { return _value; }
    std::shared_ptr<Term>& type() { return _type; }

    std::string string() const {
        return _value->string() + ":" + _type->string();
    }
    std::string repr_new() const {
        return _value->repr_new() + ":" + _type->repr_new();
    }

  private:
    std::shared_ptr<T> _value;
    std::shared_ptr<Term> _type;
};

const std::string SYMBOL_LAMBDA = (OnlyAscii ? "$" : "λ");

class AbstLambda : public Term {
  public:
    AbstLambda(std::shared_ptr<Typed<Variable>> v, std::shared_ptr<Term> e) : Term(Kind::AbstLambda), _var(v), _expr(e) {}
    AbstLambda(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e)
        : Term(Kind::AbstLambda),
          _var(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(v), t)),
          _expr(e) {}

    const std::shared_ptr<Typed<Variable>>& var() const { return _var; }
    const std::shared_ptr<Term>& expr() const { return _expr; }
    std::shared_ptr<Typed<Variable>>& var() { return _var; }
    std::shared_ptr<Term>& expr() { return _expr; }

    std::string string() const override {
        return SYMBOL_LAMBDA + _var->string() + "." + _expr->string();
    }
    std::string repr() const override {
        return "$" + _var->value()->repr() + ":(" + _var->type()->repr() + ").(" + _expr->repr() + ")";
    }
    std::string repr_new() const override {
        return "$" + _var->repr_new() + "." + _expr->repr_new();
    }

  private:
    std::shared_ptr<Typed<Variable>> _var;
    std::shared_ptr<Term> _expr;
};

const std::string SYMBOL_PI = (OnlyAscii ? "?" : "Π");

class AbstPi : public Term {
  public:
    AbstPi(std::shared_ptr<Typed<Variable>> v, std::shared_ptr<Term> e) : Term(Kind::AbstPi), _var(v), _expr(e) {}
    AbstPi(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e)
        : Term(Kind::AbstPi),
          _var(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(v), t)),
          _expr(e) {}

    const std::shared_ptr<Typed<Variable>>& var() const { return _var; }
    const std::shared_ptr<Term>& expr() const { return _expr; }
    std::shared_ptr<Typed<Variable>>& var() { return _var; }
    std::shared_ptr<Term>& expr() { return _expr; }

    std::string string() const override {
        return SYMBOL_PI + _var->string() + "." + _expr->string();
    }
    std::string repr() const override {
        return "?" + _var->value()->repr() + ":(" + _var->type()->repr() + ").(" + _expr->repr() + ")";
    }
    std::string repr_new() const override {
        return "?" + _var->repr_new() + "." + _expr->repr_new();
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
    std::vector<std::shared_ptr<Term>>& types() { return _types; }
    const std::string& name() const { return _name; }
    std::string& name() { return _name; }

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
    std::string repr_new() const override {
        std::string res(_name);
        res += "[";
        if (_types.size() > 0) res += _types[0]->repr_new();
        for (size_t i = 1; i < _types.size(); ++i) res += ", " + _types[i]->repr_new();
        res += "]";
        return res;
    }

  private:
    std::string _name;
    std::vector<std::shared_ptr<Term>> _types;
};

std::shared_ptr<Term> copy(const std::shared_ptr<Term>& term);

std::set<char> free_var(const std::shared_ptr<Term>& term);
template <class... Ts>
std::set<char> free_var(const std::shared_ptr<Term>& term, Ts... data) {
    return set_union(free_var(term), free_var(data...));
}

bool is_free_var(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& var);

std::shared_ptr<Variable> get_fresh_var(const std::shared_ptr<Term>& term);
template<class... Ts>
std::shared_ptr<Variable> get_fresh_var(const std::shared_ptr<Term>& term, Ts... data) {
    std::set<char> univ;
    for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    set_minus_inplace(univ, free_var(term, data...));
    if (!univ.empty()) return std::make_shared<Variable>(*univ.begin());
    std::cerr << "out of fresh variable" << std::endl;
    exit(EXIT_FAILURE);
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& var_bind, const std::shared_ptr<Term>& expr);
std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Term>& var_bind, const std::shared_ptr<Term>& expr);

bool exact_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b);
bool alpha_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b);

template<class T, class U>
bool alpha_comp(const std::shared_ptr<T>& a, const std::shared_ptr<U>& b) {
    return alpha_comp(std::static_pointer_cast<Term>(a), std::static_pointer_cast<Term>(b));
}

const std::string SYMBOL_EMPTY = (OnlyAscii ? "{}" : "∅");
const std::string HEADER_CONTEXT = (OnlyAscii ? "Context" : "Γ");

class Context {
  public:
    Context() : _context{} {}
    Context(const std::vector<std::shared_ptr<Typed<Variable>>>& list) : _context(list) {}
    template <class... Ts>
    Context(Ts... vals) : _context{vals...} {}

    const auto& data() const { return _context; }
    auto& data() { return _context; }

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
    std::string repr_new() const {
        std::string res("");
        res += std::to_string(_context.size()) + "\n";
        for (auto& ptr : _context) res += ptr->value()->repr_new() + " : " + ptr->type()->repr_new() + "\n";
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
               std::shared_ptr<Term> prop)
        : _context(context),
          _definiendum(constant),
          _definiens(nullptr),
          _type(prop) {}

    Definition(std::shared_ptr<Context> context,
               std::shared_ptr<Constant> constant,
               std::shared_ptr<Term> proof,
               std::shared_ptr<Term> prop)
        : _context(context),
          _definiendum(constant),
          _definiens(proof),
          _type(prop) {}

    std::string string() const {
        std::string res;
        res = (_definiens ? "Def< " : "Def-prim< ");
        res += _context->string();
        res += " " + DEFINITION_SEPARATOR + " " + _definiendum->name();
        res += " := " + (_definiens ? _definiens->string() : EMPTY_DEFINIENS);
        res += " : " + _type->string();
        res += " >";
        return res;
    }
    std::string repr() const {
        std::string res;
        res = "def2\n";
        res += _context->repr();
        res += _definiendum->name() + "\n";
        res += (_definiens ? _definiens->repr() : "#") + "\n";
        res += _type->repr() + "\n";
        res += "edef2\n";
        return res;
    }

    std::string repr_new() const {
        std::string res;
        res = "def2\n";
        res += _context->repr_new();
        res += _definiendum->name() + " := " + (_definiens ? _definiens->repr_new() : "#") + " : " + _type->repr_new() + "\n";
        res += "edef2\n";
        return res;
    }

    bool is_prim() const { return !_definiens; }
    const std::shared_ptr<Context>& context() const { return _context; }
    const std::shared_ptr<Constant>& definiendum() const { return _definiendum; }
    const std::shared_ptr<Term>& definiens() const { return _definiens; }
    const std::shared_ptr<Term>& type() const { return _type; }

    std::shared_ptr<Context>& context() { return _context; }
    std::shared_ptr<Constant>& definiendum() { return _definiendum; }
    std::shared_ptr<Term>& definiens() { return _definiens; }
    std::shared_ptr<Term>& type() { return _type; }

  private:
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
        std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
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
    std::string repr_new() const {
        std::string res = "";
        for (auto& ptr : _defs) res += ptr->repr_new() + "\n";
        res += "END\n";
        return res;
    }

    std::vector<std::shared_ptr<Definition>>& defs() { return _defs; }

  private:
    std::vector<std::shared_ptr<Definition>> _defs;
};

const std::string TURNSTILE = (OnlyAscii ? "|-" : "⊢");

class Judgement {
  public:
    Judgement(std::shared_ptr<Environment> defs,
              std::shared_ptr<Context> context,
              std::shared_ptr<Term> proof,
              std::shared_ptr<Term> prop)
        : _defs(defs), _context(context), _term(proof), _type(prop) {}
    std::string string(bool inSingleLine = true, size_t indentSize = 0) const {
        std::string res("");
        std::string indent_ex_1(indentSize, '\t');
        std::string indent_ex(inSingleLine ? 0 : indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
        res += indent_ex_1 + "Judge<<" + eol;
        res += _defs->string(inSingleLine, inSingleLine ? 0 : indentSize + 1);
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
    Book() : _judges{} {}
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
        int indentSize = 1;
        if (_judges.size() > 0) res += "\n" + _judges[0]->string(singleLine, indentSize);
        for (size_t i = 1; i < _judges.size(); ++i) res += ",\n" + _judges[i]->string(singleLine, indentSize);
        res += "\n]]";
        return res;
    }
    std::string repr() const { return "to be impl'ed"; }
    std::string repr_new() const { return "to be impl'ed"; }

  private:
    std::vector<std::shared_ptr<Judgement>> _judges;
};
