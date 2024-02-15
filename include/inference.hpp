#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "common.hpp"
#include "context.hpp"
#include "environment.hpp"
#include "lambda.hpp"

class TypeError {
  public:
    TypeError(const std::string& str, const std::shared_ptr<Term>& term, const std::shared_ptr<Context>& con);
    void puterror(std::ostream& os = std::cerr);

  private:
    std::string msg;
    std::shared_ptr<Term> term;
    std::shared_ptr<Context> con;
};

std::shared_ptr<Term> get_type(const std::shared_ptr<Term>& term, const std::shared_ptr<Environment>& delta, const std::shared_ptr<Context>& gamma);

enum class RuleType {
    Sort,
    Var,
    Weak,
    Form,
    Appl,
    Abst,
    Conv,
    Def,
    Defpr,
    Inst,
    Cp,
    Sp,
    Tp
};

std::string to_string(const RuleType type);

class Rule {
  public:
    Rule(RuleType rtype);
    RuleType rtype() const;
    int& lno();
    virtual ~Rule() = default;

  private:
    RuleType _rtype;
    int _lno = -1;
};

using RulePtr = std::shared_ptr<Rule>;

class Sort : public Rule {
  public:
    Sort();
};

class Var : public Rule {
  public:
    Var(const RulePtr& idx, const std::string& vname);
    RulePtr& idx();
    const std::string& var() const;

  private:
    RulePtr _idx;
    std::string _var;
};

class Weak : public Rule {
  public:
    Weak(const RulePtr& idx1, const RulePtr& idx2, const std::string& var);
    RulePtr& idx1();
    RulePtr& idx2();
    const std::string& var() const;

  private:
    RulePtr _idx1, _idx2;
    std::string _var;
};

class Form : public Rule {
  public:
    Form(const RulePtr& idx1, const RulePtr& idx2);
    RulePtr& idx1();
    RulePtr& idx2();

  private:
    RulePtr _idx1, _idx2;
};

class Appl : public Rule {
  public:
    Appl(const RulePtr& idx1, const RulePtr& idx2);
    RulePtr& idx1();
    RulePtr& idx2();

  private:
    RulePtr _idx1, _idx2;
};

class Abst : public Rule {
  public:
    Abst(const RulePtr& idx1, const RulePtr& idx2);
    RulePtr& idx1();
    RulePtr& idx2();

  private:
    RulePtr _idx1, _idx2;
};

class Conv : public Rule {
  public:
    Conv(const RulePtr& idx1, const RulePtr& idx2);
    RulePtr& idx1();
    RulePtr& idx2();

  private:
    RulePtr _idx1, _idx2;
};

class Def : public Rule {
  public:
    Def(const RulePtr& idx1, const RulePtr& idx2, const std::string& name);
    RulePtr& idx1();
    RulePtr& idx2();
    const std::string& name() const;

  private:
    RulePtr _idx1, _idx2;
    std::string _name;
};

class Defpr : public Rule {
  public:
    Defpr(const RulePtr& idx1, const RulePtr& idx2, const std::string& name);
    RulePtr& idx1();
    RulePtr& idx2();
    const std::string& name() const;

  private:
    RulePtr _idx1, _idx2;
    std::string _name;
};

class Inst : public Rule {
  public:
    Inst(const RulePtr& idx, const size_t& n, const std::vector<RulePtr>& k, const size_t& p);
    RulePtr& idx();
    const size_t& n() const;
    const size_t& p() const;
    std::vector<RulePtr>& k();

  private:
    RulePtr _idx;
    size_t _n, _p;
    std::vector<RulePtr> _k;
};

using Delta = std::shared_ptr<Environment>;
using Gamma = std::shared_ptr<Context>;

class DeductionError {
  public:
    DeductionError(const std::string& msg);
    void puterror(std::ostream& os = std::cerr) const;

  private:
    std::string _msg;
};

RulePtr _get_script(const std::shared_ptr<Term>& term, const Delta& delta, const Gamma& gamma);

void generate_script(RulePtr& rule, TextData& data);
