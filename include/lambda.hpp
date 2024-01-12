#pragma once

#include <iostream>
#include <memory>
#include <sstream>
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
    virtual std::string repr() const;
    virtual std::string repr_new() const;
    virtual std::string repr_book() const;

  protected:
    Term(const Kind& k) : _kind(k) {}

  private:
    Kind _kind;
};

class Star : public Term {
  public:
    Star();
    std::string string() const override;
};

class Square : public Term {
  public:
    Square();
    std::string string() const override;
    std::string repr() const override;
};

class Variable : public Term {
  public:
    Variable(char ch);
    std::string string() const override;
    const char& name() const;
    char& name();

  private:
    char _var_name;
};

class Application : public Term {
  public:
    Application(std::shared_ptr<Term> m, std::shared_ptr<Term> n);

    const std::shared_ptr<Term>& M() const;
    const std::shared_ptr<Term>& N() const;
    std::shared_ptr<Term>& M();
    std::shared_ptr<Term>& N();

    std::string string() const override;
    std::string repr() const override;
    std::string repr_new() const override;

  private:
    std::shared_ptr<Term> _M, _N;
};

template <typename T, std::enable_if_t<std::is_base_of<Term, T>::value, bool> = true>
class Typed {
  public:
    Typed(std::shared_ptr<T> val, std::shared_ptr<Term> t) : _value(val), _type(t) {}

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

class AbstLambda : public Term {
  public:
    AbstLambda(const Typed<Variable>& v, std::shared_ptr<Term> e);
    AbstLambda(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e);

    const Typed<Variable>& var() const;
    const std::shared_ptr<Term>& expr() const;
    Typed<Variable>& var();
    std::shared_ptr<Term>& expr();

    std::string string() const override;
    std::string repr() const override;
    std::string repr_new() const override;
    std::string repr_book() const override;

  private:
    Typed<Variable> _var;
    std::shared_ptr<Term> _expr;
};

class AbstPi : public Term {
  public:
    AbstPi(const Typed<Variable>& v, std::shared_ptr<Term> e);
    AbstPi(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e);

    const Typed<Variable>& var() const;
    const std::shared_ptr<Term>& expr() const;
    Typed<Variable>& var();
    std::shared_ptr<Term>& expr();

    std::string string() const override;
    std::string repr() const override;
    std::string repr_new() const override;
    std::string repr_book() const override;

  private:
    Typed<Variable> _var;
    std::shared_ptr<Term> _expr;
};

class Constant : public Term {
  public:
    Constant(const std::string& name, std::vector<std::shared_ptr<Term>> list);
    template <class... Ts>
    Constant(const std::string& name, Ts... ptrs) : Term(Kind::Constant), _name(name), _types{ptrs...} {}

    const std::vector<std::shared_ptr<Term>>& types() const;
    std::vector<std::shared_ptr<Term>>& types();
    const std::string& name() const;
    std::string& name();

    std::string string() const override;
    std::string repr() const override;
    std::string repr_new() const override;

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
template <class... Ts>
std::shared_ptr<Variable> get_fresh_var(const std::shared_ptr<Term>& term, Ts... data) {
    std::set<char> univ;
    for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    set_minus_inplace(univ, free_var(term, data...));
    if (!univ.empty()) return std::make_shared<Variable>(*univ.begin());
    std::cerr << "out of fresh variable" << std::endl;
    exit(EXIT_FAILURE);
}

std::shared_ptr<Variable> get_fresh_var(const std::vector<std::shared_ptr<Term>>& terms);

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& var_bind, const std::shared_ptr<Term>& expr);
std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Term>& var_bind, const std::shared_ptr<Term>& expr);
std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::vector<std::shared_ptr<Variable>>& vars, const std::vector<std::shared_ptr<Term>>& exprs);

// shared_ptr constructors
std::shared_ptr<Variable> variable(const char& ch);
extern std::shared_ptr<Star> star;
extern std::shared_ptr<Square> sq;
std::shared_ptr<Application> appl(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b);
std::shared_ptr<AbstLambda> lambda(const std::shared_ptr<Term>& v, const std::shared_ptr<Term>& t, const std::shared_ptr<Term>& e);
std::shared_ptr<AbstPi> pi(const std::shared_ptr<Term>& v, const std::shared_ptr<Term>& t, const std::shared_ptr<Term>& e);
std::shared_ptr<Constant> constant(const std::string& name, const std::vector<std::shared_ptr<Term>>& ts);

std::shared_ptr<Variable> variable(const std::shared_ptr<Term>& t);
std::shared_ptr<Application> appl(const std::shared_ptr<Term>& t);
std::shared_ptr<AbstLambda> lambda(const std::shared_ptr<Term>& t);
std::shared_ptr<AbstPi> pi(const std::shared_ptr<Term>& t);
std::shared_ptr<Constant> constant(const std::shared_ptr<Term>& t);

bool exact_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b);
bool alpha_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b);

template <class T, class U>
bool alpha_comp(const std::shared_ptr<T>& a, const std::shared_ptr<U>& b) {
    return alpha_comp(std::static_pointer_cast<Term>(a), std::static_pointer_cast<Term>(b));
}

class Context : public std::vector<Typed<Variable>> {
  public:
    Context();
    Context(const std::vector<Typed<Variable>>& tvars);
    std::string string() const;
    std::string repr() const;
    std::string repr_new() const;
    std::string repr_book() const;

    Context& operator+=(const Typed<Variable>& tv);
    Context operator+(const Typed<Variable>& tv);
};

class Definition {
  public:
    Definition(const Context& context,
               const std::shared_ptr<Constant>& constant,
               const std::shared_ptr<Term>& prop);

    Definition(const Context& context,
               const std::shared_ptr<Constant>& constant,
               const std::shared_ptr<Term>& proof,
               const std::shared_ptr<Term>& prop);

    std::string string() const;
    std::string repr() const;
    std::string repr_new() const;

    bool is_prim() const;
    const Context& context() const;
    const std::shared_ptr<Constant>& definiendum() const;
    const std::shared_ptr<Term>& definiens() const;
    const std::shared_ptr<Term>& type() const;

    Context& context();
    std::shared_ptr<Constant>& definiendum();
    std::shared_ptr<Term>& definiens();
    std::shared_ptr<Term>& type();

  private:
    Context _context;
    std::shared_ptr<Constant> _definiendum;
    std::shared_ptr<Term> _definiens, _type;
};

class Environment : public std::vector<Definition> {
  public:
    Environment();
    Environment(const std::vector<Definition>& defs);
    std::string string(bool inSingleLine = true, size_t indentSize = 0) const;
    std::string repr() const;
    std::string repr_new() const;
    std::string repr_book() const;

    Environment& operator+=(const Definition& def);
    Environment operator+(const Definition& def);
};

class Judgement {
  public:
    Judgement(const Environment& env,
              const Context& context,
              const std::shared_ptr<Term>& proof,
              const std::shared_ptr<Term>& prop);
    std::string string(bool inSingleLine = true, size_t indentSize = 0) const;

    const Environment& env() const;
    const Context& context() const;
    const std::shared_ptr<Term>& term() const;
    const std::shared_ptr<Term>& type() const;

    Environment& env();
    Context& context();
    std::shared_ptr<Term>& term();
    std::shared_ptr<Term>& type();

  private:
    Environment _env;
    Context _context;
    std::shared_ptr<Term> _term, _type;
};

class Book;

bool equiv_context_n(const Context& a, const Context& b, size_t n);
bool equiv_context_n(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b, size_t n);
bool equiv_context(const Context& a, const Context& b);
bool equiv_context(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b);
bool equiv_def(const Definition& a, const Definition& b);
bool equiv_def(const std::shared_ptr<Definition>& a, const std::shared_ptr<Definition>& b);
bool equiv_env(const Environment& a, const Environment& b);
bool equiv_env(const std::shared_ptr<Environment>& a, const std::shared_ptr<Environment>& b);
bool has_variable(const Context& g, const std::shared_ptr<Variable>& v);
bool has_variable(const Context& g, const std::shared_ptr<Term>& v);
bool has_variable(const Context& g, char v);
bool has_constant(const Environment& env, const std::string& name);
bool has_definition(const Environment& env, const Definition& def);
bool is_sort(const std::shared_ptr<Term>& t);
bool is_var_applicable(const Book& book, size_t idx, char var);
bool is_weak_applicable(const Book& book, size_t idx1, size_t idx2, char var);
bool is_form_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_appl_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_abst_applicable(const Book& book, size_t idx1, size_t idx2);

std::shared_ptr<Term> beta_reduce(const std::shared_ptr<Term>& term);
bool is_beta_reachable(const std::shared_ptr<Term>& from, const std::shared_ptr<Term>& to);

bool is_conv_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_def_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name);
bool is_def_prim_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name);
bool is_inst_applicable(const Book& book, size_t idx, size_t n, const std::vector<size_t>& k, size_t p);

class Book : public std::vector<Judgement> {
  public:
    Book();
    Book(const std::vector<Judgement>& list);
    template <class... Ts>
    Book(Ts... vals) : std::vector<Judgement>{vals...} {}

    // inference rules
    void sort();
    void var(size_t m, char x);
    void weak(size_t m, size_t n, char x);
    void form(size_t m, size_t n);
    void appl(size_t m, size_t n);
    void abst(size_t m, size_t n);
    void conv(size_t m, size_t n);
    void def(size_t m, size_t n, const std::string& a);
    void defpr(size_t m, size_t n, const std::string& a);
    void inst(size_t m, size_t n, const std::vector<size_t>& k, size_t p);
    // sugar syntax
    void cp(size_t m);
    void sp(size_t m, size_t n);

    std::string string() const;
    std::string repr() const;
    std::string repr_new() const;
};
