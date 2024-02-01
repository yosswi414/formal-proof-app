#include "inference.hpp"

#include "context.hpp"
#include "environment.hpp"

TypeError::TypeError(const std::string& str, const std::shared_ptr<Term>& term, const std::shared_ptr<Context>& con) : msg(str), term(term), con(con) {}

void TypeError::puterror(std::ostream& os) {
    os << BOLD(RED("TypeError")) << msg << "\n";
    os << "at term = " << term << ", context = " << con << std::endl;
}

std::shared_ptr<Term> get_type(const std::shared_ptr<Term>& term, const std::shared_ptr<Environment>& delta, const std::shared_ptr<Context>& gamma) {
    std::shared_ptr<Term> type = nullptr;
    switch (term->etype()) {
        case EpsilonType::Star:
            return sq;
        case EpsilonType::Square:
            throw TypeError("square is not typable", term, gamma);
        case EpsilonType::Variable: {
            auto t = variable(term);
            for (auto&& tv : *gamma) {
                if (tv.value()->name() == t->name()) {
                    return NF(tv.type(), delta);
                }
            }
            throw TypeError("variable not found in the context", term, gamma);
        }
        case EpsilonType::Application: {
            auto t = appl(term);
            auto MT = get_type(t->M(), delta, gamma);
            if (MT->etype() != EpsilonType::AbstPi) {
                throw TypeError("lhs of application must be a pi abstraction", term, gamma);
            }
            auto pMT = pi(MT);
            return NF(substitute(pMT->expr(), pMT->var().value(), t->N()), delta);
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            auto def = delta->lookup_def(t->name());
            auto N = def->type();
            std::vector<std::shared_ptr<Variable>> xs;
            for (auto&& tv : *def->context()) xs.push_back(tv.value());
            return NF(substitute(N, xs, t->args()), delta);
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            auto x = t->var().value();
            auto A = t->var().type();
            auto M = t->expr();
            std::shared_ptr<Term> B;
            if (is_free_var(*gamma, x)) {
                auto z = get_fresh_var(*gamma);
                B = get_type(substitute(M, x, z), delta, std::make_shared<Context>(*gamma + Typed<Variable>(z, A)));
                return NF(pi(z, A, B), delta);
            }
            B = get_type(M, delta, std::make_shared<Context>(*gamma + Typed<Variable>(x, A)));
            return NF(pi(x, A, B), delta);
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            auto x = t->var().value();
            auto A = t->var().type();
            auto B = t->expr();
            std::shared_ptr<Term> s;
            if (is_free_var(*gamma, x)) {
                auto z = get_fresh_var(*gamma);
                s = get_type(substitute(B, x, z), delta, std::make_shared<Context>(*gamma + Typed<Variable>(z, A)));
            } else {
                s = get_type(B, delta, std::make_shared<Context>(*gamma + Typed<Variable>(x, A)));
            }
            return is_sort(s) ? s : nullptr;
        }
        default:
            check_true_or_exit(
                false,
                "unknown etype: " << to_string(term->etype()),
                __FILE__, __LINE__, __func__);
    }
}

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

std::string to_string(const RuleType type) {
    switch (type) {
        case RuleType::Sort: return "sort";
        case RuleType::Var: return "var";
        case RuleType::Weak: return "weak";
        case RuleType::Form: return "form";
        case RuleType::Appl: return "appl";
        case RuleType::Abst: return "abst";
        case RuleType::Conv: return "conv";
        case RuleType::Def: return "def";
        case RuleType::Defpr: return "defpr";
        case RuleType::Inst: return "inst";
        case RuleType::Cp: return "cp";
        case RuleType::Sp: return "sp";
        case RuleType::Tp: return "tp";
    }
}

class Rule {
  public:
    Rule(RuleType type);
    RuleType type() const;

  private:
    RuleType _type;
};

Rule::Rule(RuleType type) : _type(type) {}
RuleType Rule::type() const { return _type; }

class Sort : public Rule {
  public:
    Sort();
};

Sort::Sort() : Rule(RuleType::Sort) {}

class Var : public Rule {
  public:
    Var(size_t idx, char var);
    size_t idx() const;
    char var() const;

  private:
    size_t _idx;
    char _var;
};

Var::Var(size_t idx, char var) : Rule(RuleType::Var), _idx(idx), _var(var) {}
size_t Var::idx() const { return _idx; }
char Var::var() const { return _var; }

class Weak : public Rule {
  public:
    Weak(size_t idx1, size_t idx2, char var);
    size_t idx1() const;
    size_t idx2() const;
    char var() const;

  private:
    size_t _idx1, _idx2;
    char _var;
};

Weak::Weak(size_t idx1, size_t idx2, char var) : Rule(RuleType::Weak), _idx1(idx1), _idx2(idx2), _var(var) {}
size_t Weak::idx1() const { return _idx1; }
size_t Weak::idx2() const { return _idx2; }
char Weak::var() const { return _var; }

class Form : public Rule {
  public:
    Form(size_t idx1, size_t idx2);
    size_t idx1() const;
    size_t idx2() const;

  private:
    size_t _idx1, _idx2;
};

Form::Form(size_t idx1, size_t idx2) : Rule(RuleType::Form), _idx1(idx1), _idx2(idx2) {}
size_t Form::idx1() const { return _idx1; }
size_t Form::idx2() const { return _idx2; }

class Appl : public Rule {
  public:
    Appl(size_t idx1, size_t idx2);
    size_t idx1() const;
    size_t idx2() const;

  private:
    size_t _idx1, _idx2;
};

Appl::Appl(size_t idx1, size_t idx2) : Rule(RuleType::Appl), _idx1(idx1), _idx2(idx2) {}
size_t Appl::idx1() const { return _idx1; }
size_t Appl::idx2() const { return _idx2; }

class Abst : public Rule {
  public:
    Abst(size_t idx1, size_t idx2);
    size_t idx1() const;
    size_t idx2() const;

  private:
    size_t _idx1, _idx2;
};

Abst::Abst(size_t idx1, size_t idx2) : Rule(RuleType::Abst), _idx1(idx1), _idx2(idx2) {}
size_t Abst::idx1() const { return _idx1; }
size_t Abst::idx2() const { return _idx2; }

class Conv : public Rule {
  public:
    Conv(size_t idx1, size_t idx2);
    size_t idx1() const;
    size_t idx2() const;

  private:
    size_t _idx1, _idx2;
};

Conv::Conv(size_t idx1, size_t idx2) : Rule(RuleType::Conv), _idx1(idx1), _idx2(idx2) {}
size_t Conv::idx1() const { return _idx1; }
size_t Conv::idx2() const { return _idx2; }

class Def : public Rule {
  public:
    Def(size_t idx1, size_t idx2, const std::string& name);
    size_t idx1() const;
    size_t idx2() const;
    const std::string& name() const;

  private:
    size_t _idx1, _idx2;
    std::string _name;
};

Def::Def(size_t idx1, size_t idx2, const std::string& name) : Rule(RuleType::Def), _idx1(idx1), _idx2(idx2), _name(name) {}
size_t Def::idx1() const { return _idx1; }
size_t Def::idx2() const { return _idx2; }
const std::string& Def::name() const { return _name; }

class Defpr : public Rule {
  public:
    Defpr(size_t idx1, size_t idx2, const std::string& name);
    size_t idx1() const;
    size_t idx2() const;
    const std::string& name() const;

  private:
    size_t _idx1, _idx2;
    std::string _name;
};

Defpr::Defpr(size_t idx1, size_t idx2, const std::string& name) : Rule(RuleType::Defpr), _idx1(idx1), _idx2(idx2), _name(name) {}
size_t Defpr::idx1() const { return _idx1; }
size_t Defpr::idx2() const { return _idx2; }
const std::string& Defpr::name() const { return _name; }

class Inst : public Rule {
  public:
    Inst(size_t idx, size_t n, const std::vector<size_t>& k, size_t p);
    size_t idx() const;
    size_t n() const;
    size_t p() const;
    const std::vector<size_t>& k() const;

  private:
    size_t _idx, _n, _p;
    std::vector<size_t> _k;
};

Inst::Inst(size_t idx, size_t n, const std::vector<size_t>& k, size_t p) : Rule(RuleType::Inst), _idx(idx), _n(n), _k(k), _p(p) {}
size_t Inst::idx() const { return _idx; }
size_t Inst::n() const { return _n; }
const std::vector<size_t>& Inst::k() const { return _k; }
size_t Inst::p() const { return _p; }

using Script = std::vector<std::pair<size_t, std::shared_ptr<Rule>>>;

Script get_script(const std::shared_ptr<Term>& term, const std::shared_ptr<Environment>& delta, const std::shared_ptr<Context>& gamma) {
    Script script;
}
