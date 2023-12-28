#pragma once

#include <vector>
#include <string>
#include <utility>
#include <typeinfo>

/*
note: [*] denotes a list of *
      <*> denotes a tuple
      M: N denotes that M is N
Book            [Judgement]
Judgement       <Definitions, Context, Proof: ε, Prop: ε>
Definitions     [Definition]
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

class Lambda {
  public:
    Lambda() = default;
    std::string typestr() const { return typeid(*this).name(); }
    virtual ~Lambda() = default;

  private:
};

class Variable : public Lambda {
  public:
    Variable() = default;
    Variable(char ch) : _var_name(ch) {}
    std::string name() const { return std::to_string(_var_name); }

  private:
    char _var_name;
};

class Star : public Lambda {
  public:
    Star() = default;
    std::string name() const { return "*"; }
};

class Square : public Lambda {
  public:
    Square() = default;
    std::string name() const { return "@"; }
};

class Application : public Lambda {
  public:
    Application() = default;
    Application(const Lambda& x, const Lambda& y) : _operand(x), _operator(y) {}

  private:
    Lambda _operand, _operator;
};

using TypedTerm = std::pair<Lambda, Lambda>;
using TypedVar = std::pair<Variable, Lambda>;

class AbstLambda : public Lambda {
  public:
    AbstLambda() = default;

  private:
    TypedVar _M;
    Lambda _N;
};

class AbstPi : public Lambda {
  public:
    AbstPi() = default;

  private:
    TypedVar _M;
    Lambda _N;
};

class Constant : public Lambda {
  public:
    Constant() = default;
    std::string name() const { return _const_name; }

  private:
    std::string _const_name;
    std::vector<Lambda> _arg_types;
};
