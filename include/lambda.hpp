#pragma once

#include <iostream>
#include <map>
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

enum class EpsilonType {
    Star,
    Square,
    Variable,
    Application,
    AbstLambda,
    AbstPi,
    Constant,
};

std::string to_string(const EpsilonType& k);

template <typename PtrType, typename std::enable_if_t<std::is_pointer<PtrType>::value || std::is_same<PtrType, std::unique_ptr<typename PtrType::element_type>>::value || std::is_same<PtrType, std::shared_ptr<typename PtrType::element_type>>::value, int> = 0>
std::ostream& operator<<(std::ostream& os, const PtrType& ptr) {
    return os << ptr->string();
}

std::ostream& operator<<(std::ostream& os, const EpsilonType& k);

template <typename T, typename = std::void_t<decltype(std::declval<T>().string())>>
std::ostream& operator<<(std::ostream& os, const T& x) {
    os << x.string();
    return os;
}

class Term {
  public:
    Term() = delete;
    virtual ~Term() = default;
    EpsilonType etype() const { return _etype; }
    virtual std::string string() const = 0;
    virtual std::string repr() const;
    virtual std::string repr_new() const;
    virtual std::string repr_book() const;
    virtual std::string string_db(std::vector<char> bound = {}) const;

  protected:
    Term(const EpsilonType& et) : _etype(et) {}

  private:
    EpsilonType _etype;
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
    std::string string_db(std::vector<char> bound = {}) const override;
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
    std::string repr_book() const override;
    std::string string_db(std::vector<char> bound = {}) const override;

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
    std::string string_db(std::vector<char> bound = {}) const override;

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
    std::string string_db(std::vector<char> bound = {}) const override;

  private:
    Typed<Variable> _var;
    std::shared_ptr<Term> _expr;
};

class Constant : public Term {
  public:
    Constant(const std::string& name, std::vector<std::shared_ptr<Term>> list);
    template <class... Ts>
    Constant(const std::string& name, Ts... ptrs) : Term(EpsilonType::Constant), _name(name), _args{ptrs...} {}

    const std::vector<std::shared_ptr<Term>>& args() const;
    std::vector<std::shared_ptr<Term>>& args();
    const std::string& name() const;
    std::string& name();

    std::string string() const override;
    std::string repr() const override;
    std::string repr_new() const override;
    std::string repr_book() const override;
    std::string string_db(std::vector<char> bound = {}) const override;

  private:
    std::string _name;
    std::vector<std::shared_ptr<Term>> _args;
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
template <class... Ts>
std::shared_ptr<Application> appl(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b, Ts... terms) {
    return appl(appl(a, b), terms...);
}
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

bool is_sort(const std::shared_ptr<Term>& t);

std::shared_ptr<Term> beta_reduce(const std::shared_ptr<Application>& term);
std::shared_ptr<Term> beta_nf(const std::shared_ptr<Term>& term);

bool is_beta_reducible(const std::shared_ptr<Term>& term);

class InferenceError {
  public:
    InferenceError();
    InferenceError(const std::string& str);
    void puterror(std::ostream& os = std::cerr) const;
    const std::string& str() const;
    template <typename T>
    InferenceError& operator<<(const T& rhs) {
        std::stringstream ss;
        ss << rhs;
        _msg += ss.str();
        return *this;
    }

  private:
    std::string _msg;
};
