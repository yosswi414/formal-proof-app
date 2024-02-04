#include "lambda.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <set>
#include <stack>
#include <string>

#include "common.hpp"
#include "parser.hpp"

/* [TODO]
 * - [done] tokenizer ("$x:(%(y)(z)).(*)" -> ['$'Lambda, 'x'Var, ':'Colon, '('LPar, '%'Appl, ...])
 * - [done] parser ([...] -> AbstLambda)
 *      - use Location or some sort
 * - [done] let Location have original lines reference (too tedious to tell reference every time)
 * - [done] (easy) for backward compatibility, convert def_file with new notation to the one written in conventional notation
 *      - e.g.) "$x:A.B" -> "$x:(A).(B)"
 *              "A:*" -> "A\n*"
 *              "implies := ?x:A.B : *" -> "implies\n?x:(A).(B)\n*"
 * - [done] alpha equiv
 * - [done] beta reduction
 *      essential for conv
 * - [done] delta reduction
 * - [done] get_type()
 * - get_script()
 * [IDEA]
 * - rename: AbstLambda -> Lambda, AbstPi -> Pi
 * - [done] flag: environment variable for definition
 *      flag A:* | B:* | P:S->* {
 *          def2
 *          ...
 *      }
 * - [partially done] (hard?) macro: user-defined operator
 *      - arity? associativity?
 *      #define ! * := not[*]
 *      #define x -> y := ?z:x.y
 * - variable as an index (to prevent variable name depletion)
 *      - string() |-> #123 (non-negative)
 * - variable as a string (to allow variable names such as m_1, m_2)
 */

std::string to_string(const EpsilonType& k) {
    switch (k) {
        case EpsilonType::Star:
            return "EpsilonType::Star";
        case EpsilonType::Square:
            return "EpsilonType::Square";
        case EpsilonType::Variable:
            return "EpsilonType::Variable";
        case EpsilonType::Application:
            return "EpsilonType::Application";
        case EpsilonType::AbstLambda:
            return "EpsilonType::AbstLambda";
        case EpsilonType::AbstPi:
            return "EpsilonType::AbstPi";
        case EpsilonType::Constant:
            return "EpsilonType::Constant";
        default:
            check_true_or_exit(
                false,
                "unknown EpsilonType value (value:" << (int)k << ")",
                __FILE__, __LINE__, __func__);
    }
}

std::ostream& operator<<(std::ostream& os, const EpsilonType& k) {
    return os << to_string(k);
}

std::shared_ptr<Term> copy(const std::shared_ptr<Term>& term) {
    switch (term->etype()) {
        case EpsilonType::Star:
            return star;
        case EpsilonType::Square:
            return sq;
        case EpsilonType::Variable: {
            auto t = variable(term);
            return std::make_shared<Variable>(t->name());
        }
        case EpsilonType::Application: {
            auto t = appl(term);
            return std::make_shared<Application>(copy(t->M()), copy(t->N()));
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            return std::make_shared<AbstLambda>(copy(t->var().value()), copy(t->var().type()), copy(t->expr()));
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            return std::make_shared<AbstPi>(copy(t->var().value()), copy(t->var().type()), copy(t->expr()));
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            std::vector<std::shared_ptr<Term>> args;
            for (auto&& ptr : t->args()) args.emplace_back(copy(ptr));
            return constant(t->name(), args);
        }
        default:
            check_true_or_exit(
                false,
                "unknown etype: " << to_string(term->etype()),
                __FILE__, __LINE__, __func__);
    }
}

std::set<std::string> free_var(const std::shared_ptr<Term>& term) {
    std::set<std::string> FV;
    switch (term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
            return FV;
        case EpsilonType::Variable: {
            auto t = variable(term);
            FV.insert(t->name());
            return FV;
        }
        case EpsilonType::Application: {
            auto t = appl(term);
            set_union(free_var(t->M()), free_var(t->N()), FV);
            return FV;
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            FV = free_var(t->var().type(), t->expr());
            FV.erase(t->var().value()->name());
            return FV;
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            FV = free_var(t->var().type(), t->expr());
            FV.erase(t->var().value()->name());
            return FV;
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            for (auto& arg : t->args()) set_union_inplace(FV, free_var(arg));
            return FV;
        }
        default:
            check_true_or_exit(
                false,
                "unknown etype: " << to_string(term->etype()),
                __FILE__, __LINE__, __func__);
    }
}

bool is_free_var(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& var) {
    auto fv = free_var(term);
    return fv.find(var->name()) != fv.end();
}

int _fresh_var_id = 0;
std::set<std::string> _char_vars_set;
const std::string _preferred_names = "XYZWUVxyzwpqrst";

const std::set<std::string>& char_vars_set() {
    if (_char_vars_set.empty()) {
        for (char ch = 'A'; ch <= 'Z'; ++ch) _char_vars_set.insert({ch});
        for (char ch = 'a'; ch <= 'z'; ++ch) _char_vars_set.insert({ch});
    }
    return _char_vars_set;
}

std::shared_ptr<Variable> get_fresh_var_depleted() {
    return variable("v_" + std::to_string(_fresh_var_id++));
}

std::shared_ptr<Variable> get_fresh_var(const std::shared_ptr<Term>& term) {
    // std::set<char> univ;
    // for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    // for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    // set_minus_inplace(univ, free_var(term));
    // if (!univ.empty()) return variable(*univ.begin());
    // check_true_or_exit(false, "out of fresh variable",
    //                    __FILE__, __LINE__, __func__);
    auto univ = char_vars_set();
    set_minus_inplace(univ, free_var(term));
    if (!univ.empty()) {
        for(auto&& ch : _preferred_names) {
            auto itr = univ.find({ch});
            if (itr != univ.end()) return variable({ch});
        }
        return variable(*univ.begin());
    }
    return get_fresh_var_depleted();
}

std::shared_ptr<Variable> get_fresh_var(const std::vector<std::shared_ptr<Term>>& terms) {
    // std::set<char> univ;
    // for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    // for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    // for (auto&& t : terms) set_minus_inplace(univ, free_var(t));
    // if (!univ.empty()) return variable(*univ.begin());
    // check_true_or_exit(false, "out of fresh variable",
    //                    __FILE__, __LINE__, __func__);
    auto univ = char_vars_set();
    for (auto&& t : terms) set_minus_inplace(univ, free_var(t));
    if (!univ.empty()) {
        for (auto&& ch : _preferred_names) {
            auto itr = univ.find({ch});
            if (itr != univ.end()) return variable({ch});
        }
        return variable(*univ.begin());
    }
    return get_fresh_var_depleted();
}

std::shared_ptr<Term> rename_var_short(std::shared_ptr<Term> term) {
    if (!term) return nullptr;
    switch (term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        // free variable with long name shall be processed at Environment::repr()
        case EpsilonType::Variable:
            return term;
        case EpsilonType::AbstLambda: {
            auto L = lambda(term);
            auto x = L->var().value();
            auto A = L->var().type();
            auto M = L->expr();
            if (x->name().size() == 1) {
                return lambda(x, rename_var_short(A), rename_var_short(M));
            }
            auto z = get_fresh_var(M);
            M = substitute(M, x, z);
            return lambda(z, rename_var_short(A), rename_var_short(M));
        }
        case EpsilonType::AbstPi: {
            auto L = pi(term);
            auto x = L->var().value();
            auto A = L->var().type();
            auto B = L->expr();
            if (x->name().size() == 1) {
                return pi(x, rename_var_short(A), rename_var_short(B));
            }
            auto z = get_fresh_var(B);
            B = substitute(B, x, z);
            return pi(z, rename_var_short(A), rename_var_short(B));
        }
        case EpsilonType::Application: {
            auto L = appl(term);
            auto M = L->M();
            auto N = L->N();
            return appl(rename_var_short(M), rename_var_short(N));
        }
        case EpsilonType::Constant: {
            auto L = constant(term);
            auto args = L->args();
            for (auto&& arg : args) arg = rename_var_short(arg);
            return constant(L->name(), args);
        }
    }
    return nullptr;
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& bind, const std::shared_ptr<Term>& expr) {
    switch (term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
            return term;
        case EpsilonType::Variable:
            // return alpha_comp(term, bind) ? copy(expr) : term;
            return alpha_comp(term, bind) ? expr : term;
        case EpsilonType::Application: {
            auto t = appl(term);
            return appl(
                substitute(t->M(), bind, expr),
                substitute(t->N(), bind, expr));
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            auto new_type = substitute(t->var().type(), bind, expr);
            auto old_var = t->var().value();
            if (alpha_comp(t->var().value(), bind) || !is_free_var(t->expr(), bind)) return lambda(
                old_var,
                new_type,
                t->expr());
            if (!is_free_var(expr, t->var().value())) return lambda(
                old_var,
                new_type,
                substitute(t->expr(), bind, expr));
            auto new_var = get_fresh_var(t, expr);
            return lambda(
                new_var,
                new_type,
                substitute(substitute(t->expr(), old_var, new_var), bind, expr));
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            auto new_type = substitute(t->var().type(), bind, expr);
            auto old_var = t->var().value();
            if (alpha_comp(t->var().value(), bind) || !is_free_var(t->expr(), bind)) return pi(
                old_var,
                new_type,
                t->expr());
            if (!is_free_var(expr, t->var().value())) return pi(
                old_var,
                new_type,
                substitute(t->expr(), bind, expr));
            auto new_var = get_fresh_var(t, expr);
            return pi(
                new_var,
                new_type,
                substitute(substitute(t->expr(), old_var, new_var), bind, expr));
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            std::vector<std::shared_ptr<Term>> args;
            for (auto& type : t->args()) args.emplace_back(substitute(type, bind, expr));
            return constant(t->name(), args);
        }
        default:
            check_true_or_exit(
                false,
                "unknown etype: " << to_string(term->etype()),
                __FILE__, __LINE__, __func__);
    }
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Term>& var_bind, const std::shared_ptr<Term>& expr) {
    check_true_or_exit(
        var_bind->etype() == EpsilonType::Variable,
        "var_bind etype error (expected EpsilonType::Variable, got " << to_string(var_bind->etype()) << ")",
        __FILE__, __LINE__, __func__);
    return substitute(term, variable(var_bind), expr);
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::vector<std::shared_ptr<Variable>>& vars, const std::vector<std::shared_ptr<Term>>& exprs) {
    check_true_or_exit(
        vars.size() == exprs.size(),
        "length of vars and exprs doesn't match",
        __FILE__, __LINE__, __func__);
    size_t n = vars.size();
    if (n == 0) return term;

    /* [idea] if the below holds we may be able to save variable namespace
     * x1:A1, ..., xn:An
     * M[x1:=U1,...,xn:=Un]
     * = M[x1:=z1]...[xn:=zn][z1:=U1]...[zn:=Un]
     * z1: a variable absent in {x2,...,xn}
     * zi: a variable absent in {U1,...,U{i-1},z1,...,z{i-1},x{i+1},...,xn}
     */
    // std::set<char> freshV;
    // for (char ch = 'A'; ch <= 'Z'; ++ch) freshV.insert(ch);
    // for (char ch = 'a'; ch <= 'z'; ++ch) freshV.insert(ch);
    // for (auto&& v : vars) set_minus_inplace(freshV, free_var(v));
    // for (auto&& e : exprs) set_minus_inplace(freshV, free_var(e));
    // check_true_or_exit(
    //     freshV.size() >= n,
    //     "out of fresh variable",
    //     __FILE__, __LINE__, __func__);
    std::vector<std::shared_ptr<Variable>> zs;
    for (size_t i = 0; i < n; ++i) {
        // zs.push_back(variable(*freshV.begin()));
        // freshV.erase(freshV.begin());
        zs.push_back(get_fresh_var_depleted());
    }

    auto t = term;
    for (size_t i = 0; i < n; ++i) t = substitute(t, vars[i], zs[i]);
    for (size_t i = 0; i < n; ++i) t = substitute(t, zs[i], exprs[i]);

    return t;
}

std::shared_ptr<Variable> variable(const char& ch) { return std::make_shared<Variable>(ch); }
std::shared_ptr<Variable> variable(const std::string& name) { return std::make_shared<Variable>(name); }
std::shared_ptr<Variable> variable(const std::shared_ptr<Term>& t) {
    return std::dynamic_pointer_cast<Variable>(t);
}
std::shared_ptr<Star> star = std::make_shared<Star>();
std::shared_ptr<Square> sq = std::make_shared<Square>();
std::shared_ptr<Application> appl(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b) {
    return std::make_shared<Application>(copy(a), copy(b));
}
std::shared_ptr<Application> appl(const std::shared_ptr<Term>& t) {
    return std::dynamic_pointer_cast<Application>(t);
}
std::shared_ptr<AbstLambda> lambda(const std::shared_ptr<Term>& v, const std::shared_ptr<Term>& t, const std::shared_ptr<Term>& e) {
    return std::make_shared<AbstLambda>(variable(copy(v)), copy(t), copy(e));
}
std::shared_ptr<AbstLambda> lambda(const std::shared_ptr<Term>& t) {
    return std::dynamic_pointer_cast<AbstLambda>(t);
}
std::shared_ptr<AbstPi> pi(const std::shared_ptr<Term>& v, const std::shared_ptr<Term>& t, const std::shared_ptr<Term>& e) {
    return std::make_shared<AbstPi>(variable(copy(v)), copy(t), copy(e));
}
std::shared_ptr<AbstPi> pi(const std::shared_ptr<Term>& t) {
    return std::dynamic_pointer_cast<AbstPi>(t);
}
std::shared_ptr<Constant> constant(const std::string& name, const std::vector<std::shared_ptr<Term>>& ts) {
    std::vector<std::shared_ptr<Term>> args;
    for (auto& type : ts) args.emplace_back(copy(type));
    return std::make_shared<Constant>(name, args);
}
std::shared_ptr<Constant> constant(const std::shared_ptr<Term>& t) {
    return std::dynamic_pointer_cast<Constant>(t);
}

bool alpha_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b) {
    if (a == b) return true;
    if (a->etype() != b->etype()) return false;
    switch (a->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
            return true;
        case EpsilonType::Variable: {
            auto la = variable(a);
            auto lb = variable(b);
            return la->name() == lb->name();
        }
        case EpsilonType::AbstLambda: {
            auto la = lambda(a);
            auto lb = lambda(b);
            if (!alpha_comp(la->var().type(), lb->var().type())) return false;
            auto lax = la->var().value();
            auto lbx = lb->var().value();
            if (!is_free_var(lb->expr(), lax)) return alpha_comp(
                la->expr(),
                substitute(lb->expr(), lbx, lax));
            auto z = get_fresh_var(lax, lbx, la->expr(), lb->expr());
            return alpha_comp(
                substitute(la->expr(), lax, z),
                substitute(lb->expr(), lbx, z));
        }
        case EpsilonType::AbstPi: {
            auto la = pi(a);
            auto lb = pi(b);
            if (!alpha_comp(la->var().type(), lb->var().type())) return false;
            auto lax = la->var().value();
            auto lbx = lb->var().value();
            if (!is_free_var(lb->expr(), lax)) return alpha_comp(
                la->expr(),
                substitute(lb->expr(), lbx, lax));
            auto z = get_fresh_var(lax, lbx, la->expr(), lb->expr());
            return alpha_comp(
                substitute(la->expr(), lax, z),
                substitute(lb->expr(), lbx, z));
        }
        case EpsilonType::Application: {
            auto la = appl(a);
            auto lb = appl(b);
            return alpha_comp(la->M(), lb->M()) && alpha_comp(la->N(), lb->N());
        }
        case EpsilonType::Constant: {
            auto la = constant(a);
            auto lb = constant(b);
            if (la->name() != lb->name()) return false;
            if (la->args().size() != lb->args().size()) return false;
            for (size_t idx = 0; idx < la->args().size(); ++idx) {
                if (!alpha_comp(la->args()[idx], lb->args()[idx])) return false;
            }
            return true;
        }
        default:
            check_true_or_exit(
                false,
                "alpha_comp(): unknown etype: " << to_string(a->etype()),
                __FILE__, __LINE__, __func__);
    }
}

bool exact_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b) {
    if (a->etype() != b->etype()) return false;
    switch (a->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
            return true;
        case EpsilonType::Variable: {
            auto la = variable(a);
            auto lb = variable(b);
            return la->name() == lb->name();
        }
        case EpsilonType::AbstLambda: {
            auto la = lambda(a);
            auto lb = lambda(b);
            return exact_comp(la->var().value(), lb->var().value()) && exact_comp(la->var().type(), lb->var().type()) && exact_comp(la->expr(), lb->expr());
        }
        case EpsilonType::AbstPi: {
            auto la = pi(a);
            auto lb = pi(b);
            return exact_comp(la->var().value(), lb->var().value()) && exact_comp(la->var().type(), lb->var().type()) && exact_comp(la->expr(), lb->expr());
        }
        case EpsilonType::Application: {
            auto la = appl(a);
            auto lb = appl(b);
            return exact_comp(la->M(), lb->M()) && exact_comp(la->N(), lb->N());
        }
        case EpsilonType::Constant: {
            auto la = constant(a);
            auto lb = constant(b);
            if (la->name() != lb->name()) return false;
            if (la->args().size() != lb->args().size()) return false;
            for (size_t idx = 0; idx < la->args().size(); ++idx) {
                if (!exact_comp(la->args()[idx], lb->args()[idx])) return false;
            }
            return true;
        }
        default:
            check_true_or_exit(
                false,
                "exact_comp(): unknown etype: " << to_string(a->etype()),
                __FILE__, __LINE__, __func__);
    }
}

bool is_sort(const std::shared_ptr<Term>& t) {
    return t->etype() == EpsilonType::Star || t->etype() == EpsilonType::Square;
}

std::shared_ptr<Term> beta_reduce(const std::shared_ptr<Application>& term) {
    if (term->M()->etype() != EpsilonType::AbstLambda) return term;
    auto M = lambda(term->M());
    auto N = term->N();
    return substitute(M->expr(), M->var().value(), N);
}

std::shared_ptr<Term> beta_nf(const std::shared_ptr<Term>& term) {
    switch (term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        case EpsilonType::Variable:
            return term;
        case EpsilonType::Application: {
            auto t = appl(term);
            if (is_beta_reducible(t)) return beta_nf(beta_reduce(t));
            auto s = appl(beta_nf(t->M()), beta_nf(t->N()));
            if (is_beta_reducible(s)) return beta_nf(beta_reduce(s));
            else return s;
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            return lambda(
                t->var().value(),
                beta_nf(t->var().type()),
                beta_nf(t->expr()));
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            return pi(
                t->var().value(),
                beta_nf(t->var().type()),
                beta_nf(t->expr()));
        }
        case EpsilonType::Constant: {
            auto t = constant(copy(term));
            for (auto&& arg : t->args()) arg = beta_nf(arg);
            return t;
        }
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}
bool is_beta_reducible(const std::shared_ptr<Term>& term) {
    check_true_or_ret_false_nomsg(term->etype() == EpsilonType::Application);
    auto t = appl(term);
    check_true_or_ret_false_nomsg(t->M()->etype() == EpsilonType::AbstLambda);
    return true;
}

std::string Term::repr() const { return string(); }
std::string Term::repr_new() const { return repr(); }
std::string Term::repr_book() const { return repr(); }
// std::string Term::string_db(std::vector<char> bound) const {
//     unused(bound);
//     return string();
// }

Star::Star() : Term(EpsilonType::Star) {}
std::string Star::string() const { return "*"; }

Square::Square() : Term(EpsilonType::Square) {}
const std::string SYMBOL_SQUARE = (OnlyAscii ? "@" : "□");
std::string Square::string() const { return SYMBOL_SQUARE; }
std::string Square::repr() const { return "@"; }

// Variable::Variable(char ch) : Term(EpsilonType::Variable), _var_name(ch) {}
Variable::Variable(char ch) : Term(EpsilonType::Variable), _var_name{ch} {}
Variable::Variable(const std::string& name) : Term(EpsilonType::Variable), _var_name(name) {}
// std::string Variable::string() const { return std::string(1, _var_name); }
std::string Variable::string() const { return _var_name; }
// std::string Variable::string_db(std::vector<char> bound) const {
//     for (int i = bound.size() - 1; i >= 0; --i) {
//         if (bound[i] == _var_name) return std::to_string(i);
//     }
//     return this->string();
// }
// const char& Variable::name() const { return _var_name; }
// char& Variable::name() { return _var_name; }
const std::string& Variable::name() const { return _var_name; }
std::string& Variable::name() { return _var_name; }

Application::Application(std::shared_ptr<Term> m, std::shared_ptr<Term> n) : Term(EpsilonType::Application), _M(m), _N(n) {}

const std::shared_ptr<Term>& Application::M() const { return _M; }
const std::shared_ptr<Term>& Application::N() const { return _N; }
std::shared_ptr<Term>& Application::M() { return _M; }
std::shared_ptr<Term>& Application::N() { return _N; }

std::string Application::string() const {
    return std::string("%") + _M->string() + " " + _N->string();
}
std::string Application::repr() const {
    return std::string("%(") + _M->repr() + ")(" + _N->repr() + ")";
}
std::string Application::repr_new() const {
    return std::string("%") + _M->repr_new() + " " + _N->repr_new();
}
std::string Application::repr_book() const {
    return "(" + _M->repr_book() + ")|(" + _N->repr_book() + ")";
}
// std::string Application::string_db(std::vector<char> bound) const {
//     return std::string("%") + _M->string_db(bound) + " " + _N->string_db(bound);
// }

const std::string SYMBOL_LAMBDA = (OnlyAscii ? "$" : "λ");
AbstLambda::AbstLambda(const Typed<Variable>& v, std::shared_ptr<Term> e) : Term(EpsilonType::AbstLambda), _var(v), _expr(e) {}
AbstLambda::AbstLambda(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e)
    : Term(EpsilonType::AbstLambda),
      _var(variable(v), t),
      _expr(e) {}

const Typed<Variable>& AbstLambda::var() const { return _var; }
const std::shared_ptr<Term>& AbstLambda::expr() const { return _expr; }
Typed<Variable>& AbstLambda::var() { return _var; }
std::shared_ptr<Term>& AbstLambda::expr() { return _expr; }

std::string AbstLambda::string() const {
    return SYMBOL_LAMBDA + _var.string() + "." + _expr->string();
}
std::string AbstLambda::repr() const {
    return "$" + _var.value()->repr() + ":(" + _var.type()->repr() + ").(" + _expr->repr() + ")";
}
std::string AbstLambda::repr_new() const {
    return "$" + _var.repr_new() + "." + _expr->repr_new();
}
std::string AbstLambda::repr_book() const {
    return "Lam " + _var.value()->repr_book() + ":(" + _var.type()->repr_book() + ").(" + _expr->repr_book() + ")";
}
// std::string AbstLambda::string_db(std::vector<char> bound) const {
//     std::string type = _var.type()->string_db(bound);
//     bound.push_back(_var.value()->name());
//     std::string expr = _expr->string_db(bound);
//     return SYMBOL_LAMBDA + _var.value()->string_db(bound) + ":" + type + "." + expr;
// }

const std::string SYMBOL_PI = (OnlyAscii ? "?" : "Π");
AbstPi::AbstPi(const Typed<Variable>& v, std::shared_ptr<Term> e) : Term(EpsilonType::AbstPi), _var(v), _expr(e) {}
AbstPi::AbstPi(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e)
    : Term(EpsilonType::AbstPi),
      _var(std::dynamic_pointer_cast<Variable>(v), t),
      _expr(e) {}

const Typed<Variable>& AbstPi::var() const { return _var; }
const std::shared_ptr<Term>& AbstPi::expr() const { return _expr; }
Typed<Variable>& AbstPi::var() { return _var; }
std::shared_ptr<Term>& AbstPi::expr() { return _expr; }

std::string AbstPi::string() const {
    return SYMBOL_PI + _var.string() + "." + _expr->string();
}
std::string AbstPi::repr() const {
    return "?" + _var.value()->repr() + ":(" + _var.type()->repr() + ").(" + _expr->repr() + ")";
}
std::string AbstPi::repr_new() const {
    return "?" + _var.repr_new() + "." + _expr->repr_new();
}
std::string AbstPi::repr_book() const {
    return "Pai " + _var.value()->repr_book() + ":(" + _var.type()->repr_book() + ").(" + _expr->repr_book() + ")";
}
// std::string AbstPi::string_db(std::vector<char> bound) const {
//     std::string type = _var.type()->string_db(bound);
//     bound.push_back(_var.value()->name());
//     std::string expr = _expr->string_db(bound);
//     return SYMBOL_PI + _var.value()->string_db(bound) + ":" + type + "." + expr;
// }

Constant::Constant(const std::string& name, std::vector<std::shared_ptr<Term>> list) : Term(EpsilonType::Constant), _name(name), _args(list) {}

const std::vector<std::shared_ptr<Term>>& Constant::args() const { return _args; }
std::vector<std::shared_ptr<Term>>& Constant::args() { return _args; }
const std::string& Constant::name() const { return _name; }
std::string& Constant::name() { return _name; }

std::string Constant::string() const {
    std::string res(_name);
    res += "[";
    if (_args.size() > 0) res += _args[0]->string();
    for (size_t i = 1; i < _args.size(); ++i) res += ", " + _args[i]->string();
    res += "]";
    return res;
}
std::string Constant::repr() const {
    std::string res(_name);
    res += "[";
    if (_args.size() > 0) res += "(" + _args[0]->repr() + ")";
    for (size_t i = 1; i < _args.size(); ++i) res += ",(" + _args[i]->repr() + ")";
    res += "]";
    return res;
}
std::string Constant::repr_new() const {
    std::string res(_name);
    res += "[";
    if (_args.size() > 0) res += _args[0]->repr_new();
    for (size_t i = 1; i < _args.size(); ++i) res += ", " + _args[i]->repr_new();
    res += "]";
    return res;
}
std::string Constant::repr_book() const {
    std::string res(_name);
    res += "[";
    if (_args.size() > 0) res += "(" + _args[0]->repr_book() + ")";
    for (size_t i = 1; i < _args.size(); ++i) res += ",(" + _args[i]->repr_book() + ")";
    res += "]";
    return res;
}
// std::string Constant::string_db(std::vector<char> bound) const {
//     std::string res(_name);
//     res += "[";
//     if (_args.size() > 0) res += _args[0]->string_db(bound);
//     for (size_t i = 1; i < _args.size(); ++i) res += ", " + _args[i]->string_db(bound);
//     res += "]";
//     return res;
// }

InferenceError::InferenceError() : _msg(BOLD(RED("InferenceError")) ": ") {}
InferenceError::InferenceError(const std::string& str) : _msg(str) {}
void InferenceError::puterror(std::ostream& os) const {
    os << _msg << std::endl;
}
const std::string& InferenceError::str() const { return _msg; }
