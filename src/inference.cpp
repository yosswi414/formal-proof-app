#include "inference.hpp"

#include "context.hpp"
#include "environment.hpp"
#include "judgement.hpp"

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
    Rule(RuleType type, int lno = -1);
    RuleType type() const;

  private:
    RuleType _type;
    int _lno;
};

Rule::Rule(RuleType type, int lno) : _type(type), _lno(lno) {}
RuleType Rule::type() const { return _type; }

using RulePtr = std::shared_ptr<Rule>;

class Sort : public Rule {
  public:
    Sort();
};

Sort::Sort() : Rule(RuleType::Sort) {}

class Var : public Rule {
  public:
    Var(const RulePtr& idx, const std::string& var);
    const RulePtr& idx() const;
    const std::string& var() const;

  private:
    RulePtr _idx;
    std::string _var;
};

Var::Var(const RulePtr& idx, const std::string& var) : Rule(RuleType::Var), _idx(idx), _var(var) {}
const RulePtr& Var::idx() const { return _idx; }
const std::string& Var::var() const { return _var; }

class Weak : public Rule {
  public:
    Weak(const RulePtr& idx1, const RulePtr& idx2, const std::string& var);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;
    const std::string& var() const;

  private:
    RulePtr _idx1, _idx2;
    std::string _var;
};

Weak::Weak(const RulePtr& idx1, const RulePtr& idx2, const std::string& var) : Rule(RuleType::Weak), _idx1(idx1), _idx2(idx2), _var(var) {}
const RulePtr& Weak::idx1() const { return _idx1; }
const RulePtr& Weak::idx2() const { return _idx2; }
const std::string& Weak::var() const { return _var; }

class Form : public Rule {
  public:
    Form(const RulePtr& idx1, const RulePtr& idx2);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;

  private:
    RulePtr _idx1, _idx2;
};

Form::Form(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Form), _idx1(idx1), _idx2(idx2) {}
const RulePtr& Form::idx1() const { return _idx1; }
const RulePtr& Form::idx2() const { return _idx2; }

class Appl : public Rule {
  public:
    Appl(const RulePtr& idx1, const RulePtr& idx2);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;

  private:
    RulePtr _idx1, _idx2;
};

Appl::Appl(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Appl), _idx1(idx1), _idx2(idx2) {}
const RulePtr& Appl::idx1() const { return _idx1; }
const RulePtr& Appl::idx2() const { return _idx2; }

class Abst : public Rule {
  public:
    Abst(const RulePtr& idx1, const RulePtr& idx2);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;

  private:
    RulePtr _idx1, _idx2;
};

Abst::Abst(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Abst), _idx1(idx1), _idx2(idx2) {}
const RulePtr& Abst::idx1() const { return _idx1; }
const RulePtr& Abst::idx2() const { return _idx2; }

class Conv : public Rule {
  public:
    Conv(const RulePtr& idx1, const RulePtr& idx2);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;

  private:
    RulePtr _idx1, _idx2;
};

Conv::Conv(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Conv), _idx1(idx1), _idx2(idx2) {}
const RulePtr& Conv::idx1() const { return _idx1; }
const RulePtr& Conv::idx2() const { return _idx2; }

class Def : public Rule {
  public:
    Def(const RulePtr& idx1, const RulePtr& idx2, const std::string& name);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;
    const std::string& name() const;

  private:
    RulePtr _idx1, _idx2;
    std::string _name;
};

Def::Def(const RulePtr& idx1, const RulePtr& idx2, const std::string& name) : Rule(RuleType::Def), _idx1(idx1), _idx2(idx2), _name(name) {}
const RulePtr& Def::idx1() const { return _idx1; }
const RulePtr& Def::idx2() const { return _idx2; }
const std::string& Def::name() const { return _name; }

class Defpr : public Rule {
  public:
    Defpr(const RulePtr& idx1, const RulePtr& idx2, const std::string& name);
    const RulePtr& idx1() const;
    const RulePtr& idx2() const;
    const std::string& name() const;

  private:
    RulePtr _idx1, _idx2;
    std::string _name;
};

Defpr::Defpr(const RulePtr& idx1, const RulePtr& idx2, const std::string& name) : Rule(RuleType::Defpr), _idx1(idx1), _idx2(idx2), _name(name) {}
const RulePtr& Defpr::idx1() const { return _idx1; }
const RulePtr& Defpr::idx2() const { return _idx2; }
const std::string& Defpr::name() const { return _name; }

class Inst : public Rule {
  public:
    Inst(const RulePtr& idx, const RulePtr& n, const std::vector<RulePtr>& k, const RulePtr& p);
    const RulePtr& idx() const;
    const RulePtr& n() const;
    const RulePtr& p() const;
    const std::vector<RulePtr>& k() const;

  private:
    RulePtr _idx, _n, _p;
    std::vector<RulePtr> _k;
};

Inst::Inst(const RulePtr& idx, const RulePtr& n, const std::vector<RulePtr>& k, const RulePtr& p) : Rule(RuleType::Inst), _idx(idx), _n(n), _k(k), _p(p) {}
const RulePtr& Inst::idx() const { return _idx; }
const RulePtr& Inst::n() const { return _n; }
const std::vector<RulePtr>& Inst::k() const { return _k; }
const RulePtr& Inst::p() const { return _p; }

using Delta = std::shared_ptr<Environment>;
using Gamma = std::shared_ptr<Context>;

class DeductionError {
  public:
    DeductionError(const std::string& msg, const Delta& delta, const Gamma& gamma, const std::shared_ptr<Term>& term);

  private:
    std::string _msg;
    Delta _delta;
    Gamma _gamma;
    std::shared_ptr<Term> _term;
};
DeductionError::DeductionError(const std::string& msg, const Delta& delta, const Gamma& gamma, const std::shared_ptr<Term>& term) : _msg(msg), _delta(delta), _gamma(gamma), _term(term) {}

std::map<Delta, std::map<Gamma, std::map<std::shared_ptr<Term>, RulePtr>>> hist_inf;

RulePtr _get_script(const std::shared_ptr<Term>& term, const Delta& delta, const Gamma& gamma) {
    // cache lookup by pointer address
    bool hit_d = false, hit_g = false;
    auto itr_d = hist_inf.find(delta);
    decltype(itr_d->second.begin()) itr_g;
    if (itr_d != hist_inf.end()) {
        hit_d = true;
        itr_g = itr_d->second.find(gamma);
        if (itr_g != itr_d->second.end()) {
            hit_g = true;
            auto itr_t = itr_g->second.find(term);
            if (itr_t != itr_g->second.end()) return itr_t->second;
        }
    }
    const bool full_search = true;
    // cache lookup by checking actual equivalence
    if (full_search && !hit_d) {
        for (auto&& itr = hist_inf.begin(); itr != hist_inf.end(); ++itr) {
            if (equiv_env(delta, itr->first)) {
                hit_d = true;
                itr_d = itr;
                break;
            }
        }
    }
    if (full_search && hit_d && !hit_g) {
        auto retry = itr_d->second.find(gamma);
        if (retry != itr_d->second.end()) {
            hit_g = true;
            itr_g = retry;
        }
        for (auto&& itr = itr_d->second.begin(); !hit_g && itr != itr_d->second.end(); ++itr) {
            if (equiv_context(gamma, itr->first)) {
                hit_g = true;
                itr_g = itr;
            }
        }
    }
    if (full_search && hit_g) {
        auto retry = itr_g->second.find(term);
        if (retry != itr_g->second.end()) return retry->second;
        for (auto&& itr = itr_g->second.begin(); itr != itr_g->second.end(); ++itr) {
            if (alpha_comp(term, itr->first)) return itr->second;
        }
    }

    // cache missed (body of deduction process)
    RulePtr rule;

    switch (term->etype()) {
        case EpsilonType::Star: {
            if (gamma->size() == 0) {
                if (delta->size() == 0) rule = std::make_shared<Sort>();
                else {
                    // Δ, D; {} |- * : @
                    // def := D
                    const auto& def = delta->back();
                    RulePtr left, right;
                    Delta delta_new = std::make_shared<Environment>(delta->begin(), delta->end() - 1);
                    left = get_script(term, delta_new, gamma);
                    if (def->is_prim()) {
                        // Judgement(delta, def->context(), def->type());
                        right = get_script(def->type(), delta_new, def->context());
                        rule = std::make_shared<Defpr>(left, right, def->definiendum());
                    } else {
                        // Judgement(delta, def->context(), def->definiens(), def->type());
                        right = get_script(def->definiens(), delta_new, def->context());
                        rule = std::make_shared<Def>(left, right, def->definiendum());
                    }
                    break;
                }
            }
        }
        case EpsilonType::Square:
            // square cannot have a type
            throw DeductionError("Square (type of kind) cannot be typed", delta, gamma, term);
        case EpsilonType::Variable:
        case EpsilonType::Application:
        case EpsilonType::AbstLambda:
        case EpsilonType::AbstPi:
        case EpsilonType::Constant:
        default:
            throw DeductionError("not implemented", delta, gamma, term);
    }

    // cache register
    hist_inf[delta][gamma][term] = rule;

    return rule;
}
