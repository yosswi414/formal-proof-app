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
                if (alpha_comp(tv.value(), t)) {
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
    return "[RuleType::to_string: unknown type: " + std::to_string((int)type) + "]";
}

Rule::Rule(RuleType rtype) : _rtype(rtype) {}
RuleType Rule::rtype() const { return _rtype; }
int& Rule::lno() { return _lno; }

Sort::Sort() : Rule(RuleType::Sort) {}

Var::Var(const RulePtr& idx, const std::string& vname) : Rule(RuleType::Var), _idx(idx), _var(vname) {}
RulePtr& Var::idx() { return _idx; }
const std::string& Var::var() const { return _var; }

Weak::Weak(const RulePtr& idx1, const RulePtr& idx2, const std::string& var) : Rule(RuleType::Weak), _idx1(idx1), _idx2(idx2), _var(var) {}
RulePtr& Weak::idx1() { return _idx1; }
RulePtr& Weak::idx2() { return _idx2; }
const std::string& Weak::var() const { return _var; }

Form::Form(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Form), _idx1(idx1), _idx2(idx2) {}
RulePtr& Form::idx1() { return _idx1; }
RulePtr& Form::idx2() { return _idx2; }

Appl::Appl(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Appl), _idx1(idx1), _idx2(idx2) {}
RulePtr& Appl::idx1() { return _idx1; }
RulePtr& Appl::idx2() { return _idx2; }

Abst::Abst(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Abst), _idx1(idx1), _idx2(idx2) {}
RulePtr& Abst::idx1() { return _idx1; }
RulePtr& Abst::idx2() { return _idx2; }

Conv::Conv(const RulePtr& idx1, const RulePtr& idx2) : Rule(RuleType::Conv), _idx1(idx1), _idx2(idx2) {}
RulePtr& Conv::idx1() { return _idx1; }
RulePtr& Conv::idx2() { return _idx2; }

Def::Def(const RulePtr& idx1, const RulePtr& idx2, const std::string& name) : Rule(RuleType::Def), _idx1(idx1), _idx2(idx2), _name(name) {}
RulePtr& Def::idx1() { return _idx1; }
RulePtr& Def::idx2() { return _idx2; }
const std::string& Def::name() const { return _name; }

Defpr::Defpr(const RulePtr& idx1, const RulePtr& idx2, const std::string& name) : Rule(RuleType::Defpr), _idx1(idx1), _idx2(idx2), _name(name) {}
RulePtr& Defpr::idx1() { return _idx1; }
RulePtr& Defpr::idx2() { return _idx2; }
const std::string& Defpr::name() const { return _name; }

Inst::Inst(const RulePtr& idx, const size_t& n, const std::vector<RulePtr>& k, const size_t& p) : Rule(RuleType::Inst), _idx(idx), _n(n), _p(p), _k(k) {}
RulePtr& Inst::idx() { return _idx; }
const size_t& Inst::n() const { return _n; }
std::vector<RulePtr>& Inst::k() { return _k; }
const size_t& Inst::p() const { return _p; }

DeductionError::DeductionError(const std::string& msg) : _msg(msg) {}
void DeductionError::puterror(std::ostream& os) const { os << BOLD(RED("DeductionError")) ": " << _msg << std::endl; }

std::map<std::string, RulePtr> hist_inf;

std::string hash_delta(const Delta& delta) {
    std::string str;
    // for (auto&& def : *delta) str += "@" + def->definiendum();
    if (delta && delta->size() > 0) str = delta->back()->definiendum();
    return str;
}

std::string hash_gamma(const Gamma& gamma) {
    std::string str;
    for (auto&& tv : *gamma) str += tv.value()->string() + "{" + tv.type()->string() + "}";
    return str;
}

std::string hash_term(const std::shared_ptr<Term>& term) {
    return term->string();
}

std::string hash_tuple(const Delta& delta, const Gamma& gamma, const std::shared_ptr<Term>& term) {
    return hash_delta(delta) + "|" + hash_gamma(gamma) + "|" + hash_term(term);
}

// int func_called = 0;
// int cache_hit = 0;

RulePtr _get_script(const std::shared_ptr<Term>& term, const Delta& delta, const Gamma& gamma) {
    // ++func_called;
    // ++cache_hit;
    std::string hash = hash_tuple(delta, gamma, term);
    auto itr_n = hist_inf.find(hash);
    if (itr_n != hist_inf.end()) return itr_n->second;

    // cache missed (body of deduction process)
    // --cache_hit;
    RulePtr rule;
    // std::cerr << "cache miss: " << hash << std::endl;

    switch (term->etype()) {
        case EpsilonType::Star: {
            if (gamma->size() == 0) {
                if (delta->size() == 0) rule = std::make_shared<Sort>();
                else {
                    // Δ, D; {} |- * : @
                    // def := D
                    const auto& def = delta->back();
                    RulePtr left, right;
                    Delta delta_new = std::make_shared<Environment>(*delta);
                    delta_new->pop_back();
                    left = _get_script(term, delta_new, gamma);
                    if (def->is_prim()) {
                        // Judgement(delta, def->context(), def->type());
                        right = _get_script(def->type(), delta_new, def->context());
                        rule = std::make_shared<Defpr>(left, right, def->definiendum());
                    } else {
                        // Judgement(delta, def->context(), def->definiens(), def->type());
                        right = _get_script(def->definiens(), delta_new, def->context());
                        rule = std::make_shared<Def>(left, right, def->definiendum());
                    }
                    break;
                }
            } else {
                // Δ; Γ, x:A |- * : @
                // weak
                // left: Δ; Γ |- * : @
                // right: Δ; Γ |- A : s
                std::string vname = gamma->back().value()->name();
                std::shared_ptr<Term> type = gamma->back().type();
                Gamma gamma_new = std::make_shared<Context>(*gamma);
                gamma_new->pop_back();
                RulePtr left, right;
                left = _get_script(star, delta, gamma_new);
                right = _get_script(type, delta, gamma_new);
                rule = std::make_shared<Weak>(left, right, vname);
                break;
            }
            break;
        }
        case EpsilonType::Square:
            // square cannot have a type
            throw DeductionError("Square (type of kind) cannot be typed");
        case EpsilonType::Variable: {
            auto t = variable(term);
            // de Bruijn index to be handled properly
            std::string vname = gamma->back().value()->name();
            if (vname == t->name()) {
                // Δ; Γ, x:A |- x:A
                // var (x: not in Γ )
                // idx: Δ; Γ |- A : s
                std::shared_ptr<Term> type = gamma->back().type();
                Gamma gamma_new = std::make_shared<Context>(*gamma);
                gamma_new->pop_back();
                RulePtr idx;
                idx = _get_script(type, delta, gamma_new);
                rule = std::make_shared<Var>(idx, vname);
                break;
            } else {
                // Δ; Γ, x:C |- A:B
                // weak
                // left: Δ; Γ |- A:B
                // right:Δ; Γ |- C:s
                auto C = gamma->back().type();
                Gamma gamma_new = std::make_shared<Context>(*gamma);
                gamma_new->pop_back();
                RulePtr left, right;
                left = _get_script(term, delta, gamma_new);
                right = _get_script(C, delta, gamma_new);
                rule = std::make_shared<Weak>(left, right, vname);
                break;
            }
        }
        case EpsilonType::Application: {
            auto t = appl(term);
            // Δ; Γ |- M N : B[x:=N]
            // appl
            // left: Δ; Γ |- M : ?x:A.B
            // right: Δ; Γ |- N : A
            auto M = t->M();
            auto N = t->N();
            auto A = get_type(M, delta, gamma);
            auto B = get_type(N, delta, gamma);
            RulePtr rM, rN, rA, rB, Mconv, Nconv;
            rM = _get_script(M, delta, gamma);
            rN = _get_script(N, delta, gamma);
            rA = _get_script(A, delta, gamma);
            rB = _get_script(B, delta, gamma);
            Mconv = std::make_shared<Conv>(rM, rA);
            Nconv = std::make_shared<Conv>(rN, rB);
            rule = std::make_shared<Appl>(Mconv, Nconv);
            break;
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            // Δ; Γ |- $x:A.M : ?x:A.B
            // abst
            // left: Δ; Γ, x: A |- M: B
            // right: Δ; Γ |- ?x:A.B : s
            Gamma gamma_new = std::make_shared<Context>(*gamma);
            std::shared_ptr<Variable> z = t->var().value();
            std::shared_ptr<Term> expr = t->expr();
            if (has_variable(gamma, z)) {
                z = get_fresh_var(*gamma);
                gamma_new->emplace_back(z, t->var().type());
                expr = substitute(expr, t->var().value(), z);
            } else gamma_new->push_back(t->var());
            RulePtr left, right;
            left = _get_script(expr, delta, gamma_new);
            right = _get_script(get_type(t, delta, gamma), delta, gamma);
            rule = std::make_shared<Abst>(left, right);
            break;
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            // Δ; Γ |- ?x:A.B : s2
            // form
            // left: Δ; Γ |- A : s1
            // right: Δ; Γ, x:A |- B : s2
            Gamma gamma_new = std::make_shared<Context>(*gamma);
            std::shared_ptr<Variable> z = t->var().value();
            std::shared_ptr<Term> expr = t->expr();
            if (has_variable(gamma, z)) {
                z = get_fresh_var(*gamma);
                gamma_new->emplace_back(z, t->var().type());
                expr = substitute(expr, t->var().value(), z);
            } else gamma_new->push_back(t->var());
            RulePtr left, right;
            left = _get_script(t->var().type(), delta, gamma);
            right = _get_script(expr, delta, gamma_new);
            rule = std::make_shared<Form>(left, right);
            break;
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            auto def = delta->lookup_def(t);
            if (!def) {
                throw DeductionError("Definition not found: " + t->name());
            }
            // Δ; Γ |- a(U1...) : N[x1:=U1,...]
            // inst-prim
            // left: Δ; Γ |- * : @
            // right:   Δ; Γ |- U1: A1
            //          Δ; Γ |- U2: A2[x1:=U1]
            //          Δ; Γ |- ...
            //          Δ; Γ |- Uk: Ak[x1:=U1,...,xk-1:=Uk-1]
            RulePtr left;
            std::vector<RulePtr> rights;
            left = _get_script(star, delta, gamma);
            for (size_t i = 0; i < t->args().size(); ++i) {
                const auto& U = t->args()[i];
                rights.push_back(_get_script(U, delta, gamma));
            }
            rule = std::make_shared<Inst>(left, rights.size(), rights, delta->lookup_index(t));
            break;
        }
        default:
            throw DeductionError("not implemented");
    }

    // cache register
    hist_inf[hash] = rule;

    return rule;
}

void generate_script(RulePtr& rule, TextData& data) {
    static size_t current_lno = 0;
    if (rule->lno() >= 0) return;
    switch (rule->rtype()) {
        case RuleType::Sort: {
            auto r = std::dynamic_pointer_cast<Sort>(rule);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " sort";
            data.push_back(script);
            return;
        }
        case RuleType::Var: {
            auto r = std::dynamic_pointer_cast<Var>(rule);
            generate_script(r->idx(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " var " + std::to_string(r->idx()->lno()) + " " + r->var();
            data.push_back(script);
            return;
        }
        case RuleType::Weak: {
            auto r = std::dynamic_pointer_cast<Weak>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " weak " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno()) + " " + r->var();
            data.push_back(script);
            return;
        }
        case RuleType::Form: {
            auto r = std::dynamic_pointer_cast<Form>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " form " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno());
            data.push_back(script);
            return;
        }
        case RuleType::Appl: {
            auto r = std::dynamic_pointer_cast<Appl>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " appl " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno());
            data.push_back(script);
            return;
        }
        case RuleType::Abst: {
            auto r = std::dynamic_pointer_cast<Abst>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " abst " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno());
            data.push_back(script);
            return;
        }
        case RuleType::Conv: {
            auto r = std::dynamic_pointer_cast<Conv>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " conv " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno());
            data.push_back(script);
            return;
        }
        case RuleType::Def: {
            auto r = std::dynamic_pointer_cast<Def>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " def " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno()) + " " + r->name();
            data.push_back(script);
            return;
        }
        case RuleType::Defpr: {
            auto r = std::dynamic_pointer_cast<Defpr>(rule);
            generate_script(r->idx1(), data);
            generate_script(r->idx2(), data);
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " defpr " + std::to_string(r->idx1()->lno()) + " " + std::to_string(r->idx2()->lno()) + " " + r->name();
            data.push_back(script);
            return;
        }
        case RuleType::Inst: {
            auto r = std::dynamic_pointer_cast<Inst>(rule);
            generate_script(r->idx(), data);
            for (auto&& v : r->k()) { generate_script(v, data); }
            r->lno() = current_lno++;
            std::string script = std::to_string(r->lno()) + " inst " + std::to_string(r->idx()->lno()) + " " + std::to_string(r->k().size()) + " ";
            for (auto&& v : r->k()) { script += std::to_string(v->lno()) + " "; }
            script += std::to_string(r->p());
            data.push_back(script);
            return;
        }
        case RuleType::Cp:
        case RuleType::Sp:
        case RuleType::Tp:
            throw DeductionError("generate_script(): not implemented");
    }
}
