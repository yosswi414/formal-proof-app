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
 * - beta reduction
 *      essential for conv
 * - delta reduction
 * [IDEA]
 * - rename: AbstLambda -> Lambda, AbstPi -> Pi
 * - flag: environment variable for definition
 *      flag A:* | B:* | P:S->* {
 *          def2
 *          ...
 *      }
 * - (hard?) macro: user-defined operator
 *      - arity? associativity?
 *      #define ! * := not[*]
 *      #define x -> y := ?z:x.y
 */

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
        default:
            check_true_or_exit(
                false,
                "unknown Kind value (value:" << (int)k << ")",
                __FILE__, __LINE__, __func__);
    }
}

std::ostream& operator<<(std::ostream& os, const Kind& k) {
    return os << to_string(k);
}

std::shared_ptr<Term> copy(const std::shared_ptr<Term>& term) {
    switch (term->kind()) {
        case Kind::Star:
            return star;
        case Kind::Square:
            return sq;
        case Kind::Variable: {
            auto t = variable(term);
            return std::make_shared<Variable>(t->name());
        }
        case Kind::Application: {
            auto t = appl(term);
            return std::make_shared<Application>(copy(t->M()), copy(t->N()));
        }
        case Kind::AbstLambda: {
            auto t = lambda(term);
            return std::make_shared<AbstLambda>(copy(t->var().value()), copy(t->var().type()), copy(t->expr()));
        }
        case Kind::AbstPi: {
            auto t = pi(term);
            return std::make_shared<AbstPi>(copy(t->var().value()), copy(t->var().type()), copy(t->expr()));
        }
        case Kind::Constant: {
            auto t = constant(term);
            std::vector<std::shared_ptr<Term>> args;
            for (auto&& ptr : t->args()) args.emplace_back(copy(ptr));
            return constant(t->name(), args);
        }
        default:
            check_true_or_exit(
                false,
                "unknown kind: " << to_string(term->kind()),
                __FILE__, __LINE__, __func__);
    }
}

std::set<char> free_var(const std::shared_ptr<Term>& term) {
    std::set<char> FV;
    switch (term->kind()) {
        case Kind::Star:
        case Kind::Square:
            return FV;
        case Kind::Variable: {
            auto t = variable(term);
            FV.insert(t->name());
            return FV;
        }
        case Kind::Application: {
            auto t = appl(term);
            set_union(free_var(t->M()), free_var(t->N()), FV);
            return FV;
        }
        case Kind::AbstLambda: {
            auto t = lambda(term);
            FV = free_var(t->expr());
            FV.erase(t->var().value()->name());
            return FV;
        }
        case Kind::AbstPi: {
            auto t = pi(term);
            FV = free_var(t->expr());
            FV.erase(t->var().value()->name());
            return FV;
        }
        case Kind::Constant: {
            auto t = constant(term);
            for (auto& arg : t->args()) set_union_inplace(FV, free_var(arg));
            return FV;
        }
        default:
            check_true_or_exit(
                false,
                "unknown kind: " << to_string(term->kind()),
                __FILE__, __LINE__, __func__);
    }
}

bool is_free_var(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& var) {
    auto fv = free_var(term);
    return fv.find(var->name()) != fv.end();
}

std::shared_ptr<Variable> get_fresh_var(const std::shared_ptr<Term>& term) {
    std::set<char> univ;
    for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    set_minus_inplace(univ, free_var(term));
    if (!univ.empty()) return variable(*univ.begin());
    check_true_or_exit(false, "out of fresh variable",
                       __FILE__, __LINE__, __func__);
}

std::shared_ptr<Variable> get_fresh_var(const std::vector<std::shared_ptr<Term>>& terms) {
    std::set<char> univ;
    for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    for (auto&& t : terms) set_minus_inplace(univ, free_var(t));
    if (!univ.empty()) return variable(*univ.begin());
    check_true_or_exit(false, "out of fresh variable",
                       __FILE__, __LINE__, __func__);
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& bind, const std::shared_ptr<Term>& expr) {
    switch (term->kind()) {
        case Kind::Star:
        case Kind::Square:
            return term;
        case Kind::Variable:
            return alpha_comp(term, bind) ? copy(expr) : term;
        case Kind::Application: {
            auto t = appl(term);
            return appl(
                substitute(t->M(), bind, expr),
                substitute(t->N(), bind, expr));
        }
        case Kind::AbstLambda: {
            auto t = lambda(term);
            auto new_type = substitute(t->var().type(), bind, expr);
            auto old_var = t->var().value();
            if (alpha_comp(t->var().value(), bind)) return lambda(
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
        case Kind::AbstPi: {
            auto t = pi(term);
            auto new_type = substitute(t->var().type(), bind, expr);
            auto old_var = t->var().value();
            if (alpha_comp(t->var().value(), bind)) return pi(
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
        case Kind::Constant: {
            auto t = constant(term);
            std::vector<std::shared_ptr<Term>> args;
            for (auto& type : t->args()) args.emplace_back(substitute(type, bind, expr));
            return constant(t->name(), args);
        }
        default:
            check_true_or_exit(
                false,
                "unknown kind: " << to_string(term->kind()),
                __FILE__, __LINE__, __func__);
    }
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Term>& var_bind, const std::shared_ptr<Term>& expr) {
    check_true_or_exit(
        var_bind->kind() == Kind::Variable,
        "var_bind kind error (expected Kind::Variable, got " << to_string(var_bind->kind()) << ")",
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
    std::set<char> freshV;
    for (char ch = 'A'; ch <= 'Z'; ++ch) freshV.insert(ch);
    for (char ch = 'a'; ch <= 'z'; ++ch) freshV.insert(ch);
    for (auto&& v : vars) set_minus_inplace(freshV, free_var(v));
    for (auto&& e : exprs) set_minus_inplace(freshV, free_var(e));
    check_true_or_exit(
        freshV.size() >= n,
        "out of fresh variable",
        __FILE__, __LINE__, __func__);
    std::vector<std::shared_ptr<Variable>> zs;
    for (size_t i = 0; i < n; ++i) {
        zs.push_back(variable(*freshV.begin()));
        freshV.erase(freshV.begin());
    }

    auto t = term;
    for (size_t i = 0; i < n; ++i) t = substitute(t, vars[i], zs[i]);
    for (size_t i = 0; i < n; ++i) t = substitute(t, zs[i], exprs[i]);

    return t;
}

std::shared_ptr<Variable> variable(const char& ch) { return std::make_shared<Variable>(ch); }
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
    if (a->kind() != b->kind()) return false;
    switch (a->kind()) {
        case Kind::Star:
        case Kind::Square:
            return true;
        case Kind::Variable: {
            auto la = variable(a);
            auto lb = variable(b);
            return la->name() == lb->name();
        }
        case Kind::AbstLambda: {
            auto la = lambda(a);
            auto lb = lambda(b);
            if (!alpha_comp(la->var().type(), lb->var().type())) return false;
            return alpha_comp(la->expr(), substitute(lb->expr(), lb->var().value(), la->var().value()));
        }
        case Kind::AbstPi: {
            auto la = pi(a);
            auto lb = pi(b);
            if (!alpha_comp(la->var().type(), lb->var().type())) return false;
            return alpha_comp(la->expr(), substitute(lb->expr(), lb->var().value(), la->var().value()));
        }
        case Kind::Application: {
            auto la = appl(a);
            auto lb = appl(b);
            return alpha_comp(la->M(), lb->M()) && alpha_comp(la->N(), lb->N());
        }
        case Kind::Constant: {
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
                "alpha_comp(): unknown kind: " << to_string(a->kind()),
                __FILE__, __LINE__, __func__);
    }
}

bool exact_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b) {
    if (a->kind() != b->kind()) return false;
    switch (a->kind()) {
        case Kind::Star:
        case Kind::Square:
            return true;
        case Kind::Variable: {
            auto la = variable(a);
            auto lb = variable(b);
            return la->name() == lb->name();
        }
        case Kind::AbstLambda: {
            auto la = lambda(a);
            auto lb = lambda(b);
            return exact_comp(la->var().value(), lb->var().value()) && exact_comp(la->var().type(), lb->var().type()) && exact_comp(la->expr(), lb->expr());
        }
        case Kind::AbstPi: {
            auto la = pi(a);
            auto lb = pi(b);
            return exact_comp(la->var().value(), lb->var().value()) && exact_comp(la->var().type(), lb->var().type()) && exact_comp(la->expr(), lb->expr());
        }
        case Kind::Application: {
            auto la = appl(a);
            auto lb = appl(b);
            return exact_comp(la->M(), lb->M()) && exact_comp(la->N(), lb->N());
        }
        case Kind::Constant: {
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
                "exact_comp(): unknown kind: " << to_string(a->kind()),
                __FILE__, __LINE__, __func__);
    }
}

bool equiv_context_n(const Context& a, const Context& b, size_t n) {
    check_true_or_ret_false(
        n <= a.size(),
        "equiv_context_n(): 1st context doesn't have n statements",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        n <= b.size(),
        "equiv_context_n(): 2nd context doesn't have n statements",
        __FILE__, __LINE__, __func__);
    for (size_t i = 0; i < n; ++i) {
        check_true_or_ret_false(
            alpha_comp(a[i].value(), b[i].value()),
            "equiv_context_n(): "
                << "the variable of " << i << "-th statement doesn't match" << std::endl
                << "var 1: " << a[i].value() << std::endl
                << "var 2: " << b[i].value(),
            __FILE__, __LINE__, __func__);
        check_true_or_ret_false(
            alpha_comp(a[i].type(), b[i].type()),
            "equiv_context_n(): "
                << "the type of " << i << "-th statement doesn't match" << std::endl
                << "type 1: " << a[i].type() << std::endl
                << "type 2: " << b[i].type(),
            __FILE__, __LINE__, __func__);
    }
    return true;
}

bool equiv_context_n(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b, size_t n) {
    return equiv_context_n(*a, *b, n);
}

bool equiv_context(const Context& a, const Context& b) {
    check_true_or_ret_false(
        a.size() == b.size(),
        "equiv_context(): size of two context doesn't match",
        __FILE__, __LINE__, __func__);
    return equiv_context_n(a, b, a.size());
}

bool equiv_context(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b) {
    return equiv_context(*a, *b);
}

bool equiv_def(const Definition& a, const Definition& b) {
    check_true_or_ret_false(
        equiv_context(a.context(), b.context()),
        "equiv_def(): context doesn't match",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        a.definiendum() == b.definiendum(),
        "equiv_def(): definiendum doesn't match",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        a.is_prim() == b.is_prim(),
        "equiv_def(): one is primitive definition and another is not",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        a.is_prim() || alpha_comp(a.definiens(), b.definiens()),
        "equiv_def(): definiens doesn't match",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        alpha_comp(a.type(), b.type()),
        "equiv_def(): type doesn't match",
        __FILE__, __LINE__, __func__);
    return true;
}

bool equiv_def(const std::shared_ptr<Definition>& a, const std::shared_ptr<Definition>& b) {
    return equiv_def(*a, *b);
}

bool equiv_env(const Environment& a, const Environment& b) {
    check_true_or_ret_false(
        a.size() == b.size(),
        "equiv_env(): # of definitions doesn't match",
        __FILE__, __LINE__, __func__);
    for (size_t i = 0; i < a.size(); ++i) {
        check_true_or_ret_false(
            equiv_def(a[i], b[i]),
            "equiv_env(): "
                << "the " << i << "-th definition of environment doesn't match" << std::endl
                << "def 1: " << a[i] << std::endl
                << "def 2: " << b[i],
            __FILE__, __LINE__, __func__);
    }
    return true;
}

bool equiv_env(const std::shared_ptr<Environment>& a, const std::shared_ptr<Environment>& b) {
    return equiv_env(*a, *b);
}

bool has_variable(const Context& g, const std::shared_ptr<Variable>& v) {
    for (auto&& tv : g) {
        if (alpha_comp(tv.value(), v)) return true;
    }
    return false;
}

bool has_variable(const Context& g, const std::shared_ptr<Term>& v) {
    return has_variable(g, variable(v));
}

bool has_variable(const Context& g, char v) {
    return has_variable(g, std::make_shared<Variable>(v));
}

bool has_constant(const Environment& env, const std::string& name) {
    for (auto&& def : env) {
        if (def.definiendum() == name) return true;
    }
    return false;
}

bool has_definition(const Environment& env, const Definition& def) {
    for (auto&& d : env) {
        if (equiv_def(d, def)) return true;
    }
    return false;
}

bool is_sort(const std::shared_ptr<Term>& t) {
    return t->kind() == Kind::Star || t->kind() == Kind::Square;
}

bool is_var_applicable(const Book& book, size_t idx, char var) {
    auto& judge = book[idx];
    check_true_or_ret_false(
        is_sort(judge.type()),
        "type of judgement is neither * nor @"
            << std::endl
            << "type: " << judge.type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        !has_variable(judge.context(), var),
        "context already has a variable " << var,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_weak_applicable(const Book& book, size_t idx1, size_t idx2, char var) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        equiv_context(judge1.context(), judge2.context()),
        "context doesn't match"
            << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        is_sort(judge2.type()),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        !has_variable(judge1.context(), var),
        "context already has a variable " << var,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_form_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        equiv_context_n(judge1.context(), judge2.context(), judge1.context().size()),
        "first " << judge1.context().size() << " statements of context doesn't match" << std::endl
                 << "context 1: " << judge1.context() << std::endl
                 << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        judge1.context().size() + 1 == judge2.context().size(),
        "size of context is not appropriate"
            << std::endl
            << "size 1: " << judge1.context().size() << std::endl
            << "size 2: " << judge2.context().size() << " (should have 1 more)",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        alpha_comp(judge1.term(), judge2.context().back().type()),
        "term of 1st judge and type of last statement of context doesn't match"
            << std::endl
            << "term: " << judge1.term() << std::endl
            << "type: " << judge2.context().back().type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        is_sort(judge1.type()),
        "type of 1st judgement is neither * nor @"
            << std::endl
            << "type: " << judge1.type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        is_sort(judge2.type()),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_appl_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        equiv_context(judge1.context(), judge2.context()),
        "context doesn't match"
            << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        judge1.type()->kind() == Kind::AbstPi,
        "type of 1st judgement is not a pi abstraction"
            << std::endl
            << "type: " << judge1.type(),
        __FILE__, __LINE__, __func__);
    auto p = pi(judge1.type());
    check_true_or_ret_false(
        alpha_comp(p->var().type(), judge2.type()),
        "type of bound variable is not alpha-equivalent to the type of 2nd judgement"
            << std::endl
            << "type pi: " << p->var().type() << std::endl
            << "type  2: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_abst_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "abst: "
            << "environment doesn't match" << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        equiv_context_n(judge1.context(), judge2.context(), judge2.context().size()),
        "abst: "
            << "first " << judge2.context().size() << " statements of context doesn't match" << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        judge1.context().size() == judge2.context().size() + 1,
        "abst: "
            << "size of context is not appropriate" << std::endl
            << "size 1: " << judge1.context().size() << std::endl
            << "size 2: " << judge2.context().size() << " (should have 1 less)",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        judge2.term()->kind() == Kind::AbstPi,
        "abst: "
            << "term of 2nd judgement is not a pi abstraction" << std::endl
            << "term: " << judge2.term(),
        __FILE__, __LINE__, __func__);
    auto p = pi(judge2.term());
    auto x = judge1.context().back().value();
    auto A = judge1.context().back().type();
    auto B = judge1.type();
    check_true_or_ret_false(
        alpha_comp(p->var().value(), x),
        "abst: "
            << "bound variable is not alpha-equivalent to the last variable in context" << std::endl
            << "bound: " << p->var().value() << std::endl
            << "    x: " << x,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        alpha_comp(p->var().type(), A),
        "abst: "
            << "type of bound variable is not alpha-equivalent to the last type in context" << std::endl
            << "bound type: " << p->var().type() << std::endl
            << " last type: " << A,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        alpha_comp(p->expr(), B),
        "abst: "
            << "expr of pi abstraction is not alpha-equivalent to the type of 1st judgement" << std::endl
            << "expr pi: " << p->expr() << std::endl
            << " type 1: " << B,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        is_sort(judge2.type()),
        "abst: "
            << "type of 2nd judgement is not a sort" << std::endl
            << "type: " << judge2.type() << std::endl,
        __FILE__, __LINE__, __func__);
    return true;
}

std::shared_ptr<Term> beta_reduce(const std::shared_ptr<Application>& term) {
    if (term->M()->kind() != Kind::AbstLambda) return term;
    auto M = lambda(term->M());
    auto N = term->N();
    return substitute(M->expr(), M->var().value(), N);
}
std::shared_ptr<Term> delta_reduce(const std::shared_ptr<Constant>& term, const Environment& delta) {
    auto D = delta.lookup_def(term);

    if (D.is_prim()) return term;

    auto M = D.definiens();
    std::vector<std::shared_ptr<Variable>> xs;
    for (size_t i = 0; i < D.context().size(); ++i) {
        xs.push_back(D.context()[i].value());
    }

    return substitute(M, xs, term->args());
}

std::shared_ptr<Term> delta_nf(const std::shared_ptr<Term>& term, const Environment& delta) {
    switch(term->kind()){
        case Kind::Star:
        case Kind::Square:
        case Kind::Variable:
            return term;
        case Kind::Application:{
            auto t = appl(term);
            return appl(
                delta_nf(t->M(), delta),
                delta_nf(t->N(), delta));
        }
        case Kind::AbstLambda: {
            auto t = lambda(term);
            return lambda(
                t->var().value(),
                delta_nf(t->var().type(), delta),
                delta_nf(t->expr(), delta));
        }
        case Kind::AbstPi: {
            auto t = pi(term);
            return pi(
                t->var().value(),
                delta_nf(t->var().type(), delta),
                delta_nf(t->expr(), delta));
        }
        case Kind::Constant:{
            auto t = constant(term);
            return delta_nf(delta_reduce(t, delta), delta);
        }
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}

std::shared_ptr<Term> beta_nf(const std::shared_ptr<Term>& term) {
    switch (term->kind()) {
        case Kind::Star:
        case Kind::Square:
        case Kind::Variable:
            return term;
        case Kind::Application: {
            auto t = appl(term);
            if (is_beta_reducible(t)) return beta_nf(beta_reduce(t));
            auto s = appl(beta_nf(t->M()), beta_nf(t->N()));
            if (is_beta_reducible(s)) return beta_nf(beta_reduce(s));
            else return s;
        }
        case Kind::AbstLambda: {
            auto t = lambda(term);
            return lambda(
                t->var().value(),
                beta_nf(t->var().type()),
                beta_nf(t->expr()));
        }
        case Kind::AbstPi: {
            auto t = pi(term);
            return pi(
                t->var().value(),
                beta_nf(t->var().type()),
                beta_nf(t->expr()));
        }
        case Kind::Constant: {
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
    check_true_or_ret_false_nomsg(term->kind() == Kind::Application);
    auto t = appl(term);
    check_true_or_ret_false_nomsg(t->M()->kind() == Kind::AbstLambda);
    return true;
}

bool is_convertible(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b, const Environment& delta) {
    // naive
    // return alpha_comp(
    //     beta_nf(delta_nf(a, delta)),
    //     beta_nf(delta_nf(b, delta)));
    if (a->kind() == b->kind()) {
        switch (a->kind()) {
            case Kind::Star:
            case Kind::Square:
            case Kind::Variable:
                return alpha_comp(a, b);
            case Kind::Application: {
                auto aa = appl(a);
                auto ab = appl(b);
                auto M = aa->M();
                auto N = aa->N();
                auto K = ab->M();
                auto L = ab->N();
                if (alpha_comp(M, K)) return is_convertible(N, L, delta);
                if (alpha_comp(N, L)) return is_convertible(M, K, delta);
                if (is_convertible(M, K, delta) &&
                    is_convertible(N, L, delta)) return true;
                if (is_beta_reducible(aa)) return is_convertible(beta_reduce(aa), ab, delta);
                if (is_beta_reducible(ab)) return is_convertible(aa, beta_reduce(ab), delta);
                return false;
            }
            case Kind::AbstLambda: {
                auto la = lambda(a);
                auto lb = lambda(b);
                auto x = la->var().value();
                auto M = la->var().type();
                auto N = la->expr();
                auto y = lb->var().value();
                auto K = lb->var().type();
                auto L = substitute(lb->expr(), y, x);
                if (alpha_comp(M, K)) return is_convertible(N, L, delta);
                if (alpha_comp(N, L)) return is_convertible(M, K, delta);
                if (is_convertible(M, K, delta) &&
                    is_convertible(N, L, delta)) return true;
                return false;
            }
            case Kind::AbstPi: {
                auto pa = pi(a);
                auto pb = pi(b);
                auto x = pa->var().value();
                auto M = pa->var().type();
                auto N = pa->expr();
                auto y = pb->var().value();
                auto K = pb->var().type();
                auto L = substitute(pb->expr(), y, x);
                if (alpha_comp(M, K)) return is_convertible(N, L, delta);
                if (alpha_comp(N, L)) return is_convertible(M, K, delta);
                if (is_convertible(M, K, delta) &&
                    is_convertible(N, L, delta)) return true;
                return false;
            }
            case Kind::Constant: {
                auto ca = constant(a);
                auto cb = constant(b);
                if (ca->name() == cb->name()) {
                    check_true_or_ret_false_nomsg(ca->args().size() == cb->args().size());
                    for (size_t i = 0; i < ca->args().size(); ++i) {
                        if(!is_convertible(ca->args()[i], cb->args()[i], delta)) {
                            return is_convertible(delta_reduce(ca, delta), delta_reduce(cb, delta), delta);
                        }
                    }
                    return true;
                }
                int ai = delta.lookup_index(ca);
                int bi = delta.lookup_index(cb);
                if (ai < bi) return is_convertible(
                    ca,
                    delta_reduce(cb, delta),
                    delta);
                else return is_convertible(
                    delta_reduce(ca, delta),
                    cb,
                    delta);
            }
            default:
                check_true_or_exit(
                    false,
                    "unknown kind (value = " << (int)(a->kind()) << ")",
                    __FILE__, __LINE__, __func__);
        }
    }
    // kind doesn't match
    if (b->kind() == Kind::Constant) return is_convertible(a, delta_reduce(constant(b), delta), delta);
    switch (a->kind()) {
        case Kind::Star:
        case Kind::Square:
        case Kind::Variable:
        case Kind::AbstLambda:
        case Kind::AbstPi: {
            switch (b->kind()) {
                case Kind::Star:
                case Kind::Square:
                case Kind::Variable:
                case Kind::AbstLambda:
                case Kind::AbstPi:
                    return false;
                case Kind::Application:
                    if (is_beta_reducible(b)) return is_convertible(a, beta_reduce(appl(b)), delta);
                    return false;
                case Kind::Constant:
                    check_true_or_exit(
                        false,
                        "contradiction; this line is logically unreachable. this must be a bug",
                        __FILE__, __LINE__, __func__);
                default:
                    check_true_or_exit(
                        false,
                        "unknown kind (value = " << (int)(b->kind()) << ")",
                        __FILE__, __LINE__, __func__);
            }
        }
        case Kind::Application:
            if (is_beta_reducible(a)) return is_convertible(beta_reduce(appl(a)), b, delta);
            return false;
        case Kind::Constant:
            return is_convertible(delta_reduce(constant(a), delta), b, delta);
        default:
            check_true_or_exit(
                false,
                "unknown kind (value = " << (int)(a->kind()) << ")",
                __FILE__, __LINE__, __func__);
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}

bool is_conv_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        equiv_context(judge1.context(), judge2.context()),
        "context doesn't match"
            << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    auto B1 = judge1.type();
    auto B2 = judge2.term();
    auto s = judge2.type();
    check_true_or_ret_false(
        is_convertible(B1, B2, judge1.env()),
        "type of 1st judgement and term of 2nd judgement are not beta-delta-equivalent"
            << std::endl
            << "type 1: " << B1 << std::endl
            << "term 2: " << B2,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        is_sort(s),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << s,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_def_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        !has_constant(judge1.env(), name),
        "environment of 1st judgement already has the definition of \""
            << name << "\"" << std::endl
            << "env: " << judge1.env(),
        __FILE__, __LINE__, __func__);
    ;
    return true;
}

bool is_def_prim_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true_or_ret_false(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        !has_constant(judge1.env(), name),
        "environment of 1st judgement already has the definition of \""
            << name << "\"" << std::endl
            << "env: " << judge1.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        is_sort(judge2.type()),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_inst_applicable(const Book& book, size_t idx, size_t n, const std::vector<size_t>& k, size_t p) {
    auto& judge = book[idx];

    check_true_or_ret_false(
        k.size() == n,
        "length of k and n doesn't match (this seems to be a bug. please report with your input)"
            << std::endl
            << "length of k: " << k.size() << " (should be n = " << n << ")",
        __FILE__, __LINE__, __func__);
    for (size_t i = 0; i < n; ++i) {
        check_true_or_ret_false(
            equiv_env(judge.env(), book[k[i]].env()),
            "environment doesn't match"
                << std::endl
                << "env [" << idx << "]: " << judge.env() << std::endl
                << "env [" << k[i] << "]: " << book[k[i]].env(),
            __FILE__, __LINE__, __func__);
        check_true_or_ret_false(
            equiv_context(judge.context(), book[k[i]].context()),
            "context doesn't match"
                << std::endl
                << "context [" << idx << "]: " << judge.context() << std::endl
                << "context [" << k[i] << "]: " << book[k[i]].context(),
            __FILE__, __LINE__, __func__);
    }

    // check_true_or_ret_false(judge.context().size() == n,

    auto& D = judge.env()[p];

    std::vector<std::shared_ptr<Term>> Us;
    std::vector<std::shared_ptr<Variable>> xs;
    for (size_t i = 0; i < n; ++i) {
        auto A = D.context()[i].type();
        auto V = book[k[i]].type();
        // check V == A[xs := Us]
        auto AxU = substitute(A, xs, Us);
        check_true_or_ret_false(
            alpha_comp(V, AxU),
            "type equivalence (U_i : A_i[x_1:=U_1,..] for all i) doesn't hold"
                << std::endl
                << "k_i: " << k[i] << std::endl
                << "A[x:=U] (V should be): " << AxU << std::endl
                << "      V (actual type): " << V << std::endl
                << "                    A: " << A << std::endl
                << "                    i: " << i << std::endl
                << "                    xs: " << to_string(xs) << std::endl
                << "                    Us: " << to_string(Us) << std::endl
                << "  the def failed inst: " << D << std::endl,
            __FILE__, __LINE__, __func__);
        xs.push_back(D.context()[i].value());
        Us.push_back(book[k[i]].term());
    }

    check_true_or_ret_false(
        judge.term()->kind() == Kind::Star,
        "term of 1st judgement is not *"
            << std::endl
            << "term: " << judge.term(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        judge.type()->kind() == Kind::Square,
        "type of 1st judgement is not @"
            << std::endl
            << "type: " << judge.type(),
        __FILE__, __LINE__, __func__);

    return true;
}

std::string Term::repr() const { return string(); }
std::string Term::repr_new() const { return repr(); }
std::string Term::repr_book() const { return repr(); }

Star::Star() : Term(Kind::Star) {}
std::string Star::string() const { return "*"; }

Square::Square() : Term(Kind::Square) {}
const std::string SYMBOL_SQUARE = (OnlyAscii ? "@" : "□");
std::string Square::string() const { return SYMBOL_SQUARE; }
std::string Square::repr() const { return "@"; }

Variable::Variable(char ch) : Term(Kind::Variable), _var_name(ch) {}
std::string Variable::string() const { return std::string(1, _var_name); }
const char& Variable::name() const { return _var_name; }
char& Variable::name() { return _var_name; }

Application::Application(std::shared_ptr<Term> m, std::shared_ptr<Term> n) : Term(Kind::Application), _M(m), _N(n) {}

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

const std::string SYMBOL_LAMBDA = (OnlyAscii ? "$" : "λ");
AbstLambda::AbstLambda(const Typed<Variable>& v, std::shared_ptr<Term> e) : Term(Kind::AbstLambda), _var(v), _expr(e) {}
AbstLambda::AbstLambda(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e)
    : Term(Kind::AbstLambda),
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

const std::string SYMBOL_PI = (OnlyAscii ? "?" : "Π");
AbstPi::AbstPi(const Typed<Variable>& v, std::shared_ptr<Term> e) : Term(Kind::AbstPi), _var(v), _expr(e) {}
AbstPi::AbstPi(std::shared_ptr<Term> v, std::shared_ptr<Term> t, std::shared_ptr<Term> e)
    : Term(Kind::AbstPi),
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

Constant::Constant(const std::string& name, std::vector<std::shared_ptr<Term>> list) : Term(Kind::Constant), _name(name), _args(list) {}

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

const std::string SYMBOL_EMPTY = (OnlyAscii ? "{}" : "∅");
const std::string HEADER_CONTEXT = (OnlyAscii ? "Context" : "Γ");

Context::Context() {}
Context::Context(const std::vector<Typed<Variable>>& tvars) : std::vector<Typed<Variable>>(tvars) {}
std::string Context::string() const {
    std::string res("");
    if (this->size() == 0) return SYMBOL_EMPTY;
    res += HEADER_CONTEXT + "{";
    if (this->size() > 0) res += (*this)[0].string();
    for (size_t i = 1; i < this->size(); ++i) res += ", " + (*this)[i].string();
    res += "}";
    return res;
}
std::string Context::repr() const {
    std::string res("");
    res += std::to_string(this->size()) + "\n";
    for (auto&& tv : *this) res += tv.value()->repr() + "\n" + tv.type()->repr() + "\n";
    return res;
}
std::string Context::repr_new() const {
    std::string res("");
    res += std::to_string(this->size()) + "\n";
    for (auto&& tv : *this) res += tv.value()->repr_new() + " : " + tv.type()->repr_new() + "\n";
    return res;
}
std::string Context::repr_book() const {
    std::stringstream ss;
    if (this->size() > 0) ss << (*this)[0].value()->repr_book() << ":" << (*this)[0].type()->repr_book();
    for (size_t i = 1; i < this->size(); ++i) ss << ", " << (*this)[i].value()->repr_book() << ":" << (*this)[i].type()->repr_book();
    return ss.str();
}

Context& Context::operator+=(const Typed<Variable>& tv) {
    this->emplace_back(tv);
    return *this;
}

Context Context::operator+(const Typed<Variable>& tv) {
    return Context(*this) += tv;
}

const std::string DEFINITION_SEPARATOR = (OnlyAscii ? "|>" : "▷");
const std::string EMPTY_DEFINIENS = (OnlyAscii ? "#" : "⫫");

Definition::Definition(const Context& context,
                       const std::string& cname,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(cname),
      _definiens(nullptr),
      _type(prop) {}

Definition::Definition(const Context& context,
                       const std::string& cname,
                       const std::shared_ptr<Term>& proof,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(cname),
      _definiens(proof),
      _type(prop) {}

Definition::Definition(const Context& context,
                       const std::shared_ptr<Constant>& constant,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(constant->name()),
      _definiens(nullptr),
      _type(prop) {}

Definition::Definition(const Context& context,
                       const std::shared_ptr<Constant>& constant,
                       const std::shared_ptr<Term>& proof,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(constant->name()),
      _definiens(proof),
      _type(prop) {}

std::string Definition::string() const {
    std::string res;
    res = (_definiens ? "Def< " : "Def-prim< ");
    res += _context.string();
    res += " " + DEFINITION_SEPARATOR + " " + _definiendum;
    res += " := " + (_definiens ? _definiens->string() : EMPTY_DEFINIENS);
    res += " : " + _type->string();
    res += " >";
    return res;
}
std::string Definition::repr() const {
    std::string res;
    res = "def2\n";
    res += _context.repr();
    res += _definiendum + "\n";
    res += (_definiens ? _definiens->repr() : "#") + "\n";
    res += _type->repr() + "\n";
    res += "edef2\n";
    return res;
}

std::string Definition::repr_new() const {
    std::string res;
    res = "def2\n";
    res += _context.repr_new();
    res += _definiendum + " := " + (_definiens ? _definiens->repr_new() : "#") + " : " + _type->repr_new() + "\n";
    res += "edef2\n";
    return res;
}

std::string Definition::repr_book() const {
    std::stringstream ss;
    ss << _context.repr_book() << " |> ";
    ss << _definiendum << " := ";
    ss << (_definiens ? _definiens->repr_book() : "#") << " : ";
    ss << _type->repr_book();
    return ss.str();
}

bool Definition::is_prim() const { return !_definiens; }
const Context& Definition::context() const { return _context; }
const std::string& Definition::definiendum() const { return _definiendum; }
const std::shared_ptr<Term>& Definition::definiens() const { return _definiens; }
const std::shared_ptr<Term>& Definition::type() const { return _type; }

Context& Definition::context() { return _context; }
std::string& Definition::definiendum() { return _definiendum; }
std::shared_ptr<Term>& Definition::definiens() { return _definiens; }
std::shared_ptr<Term>& Definition::type() { return _type; }

const std::string HEADER_ENV = (OnlyAscii ? "Env" : "Δ");

Environment::Environment() {}
Environment::Environment(const std::vector<Definition>& defs) : std::vector<Definition>(defs) {
    for (size_t idx = 0; idx < this->size(); ++idx) {
        _def_index[(*this)[idx].definiendum()] = idx;
    }
}

Environment::Environment(const std::string& fname) {
    *this = parse_defs(tokenize(FileData(fname)));
}

std::string Environment::string(bool inSingleLine, size_t indentSize) const {
    std::string res = "";
    std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    if (this->size() == 0) return indent_ex + SYMBOL_EMPTY;
    res += indent_ex + HEADER_ENV + "{{" + eol;
    if (this->size() > 0) res += indent_ex + indent_in + (*this)[0].string();
    for (size_t i = 1; i < this->size(); ++i) res += "," + eol + indent_ex + indent_in + (*this)[i].string();
    res += eol + indent_ex + "}}";
    return res;
}

std::string Environment::string_brief(bool inSingleLine, size_t indentSize) const {
    std::string res = "";
    std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    if (this->size() == 0) return indent_ex + SYMBOL_EMPTY;
    res += indent_ex + HEADER_ENV + "{{" + eol;
    if (this->size() > 0) res += indent_ex + indent_in + (*this)[0].definiendum();
    for (size_t i = 1; i < this->size(); ++i) res += "," + eol + indent_ex + indent_in + (*this)[i].definiendum();
    res += eol + indent_ex + "}}";
    return res;
}

std::string Environment::repr() const {
    std::string res = "";
    for (auto&& def : *this) res += def.repr() + "\n";
    res += "END\n";
    return res;
}
std::string Environment::repr_new() const {
    std::string res = "";
    for (auto&& def : *this) res += def.repr_new() + "\n";
    res += "END\n";
    return res;
}

int Environment::lookup_index(const std::string& cname) const {
    if (_def_index.find(cname) != _def_index.end()) return _def_index.at(cname);
    for (size_t idx = 0; idx < this->size(); ++idx) {
        if ((*this)[idx].definiendum() == cname) {
            return _def_index[cname] = idx;
        }
    }
    check_true_or_exit(
        false,
        "constant \"" << cname << "\" not found in env",
        __FILE__, __LINE__, __func__);
}
int Environment::lookup_index(const std::shared_ptr<Constant>& c) const {
    return lookup_index(c->name());
}

const Definition& Environment::lookup_def(const std::string& cname) const {
    return (*this)[lookup_index(cname)];
}
const Definition& Environment::lookup_def(const std::shared_ptr<Constant>& c) const {
    return lookup_def(c->name());
}

Environment& Environment::operator+=(const Definition& def) {
    this->push_back(def);
    _def_index[def.definiendum()] = this->size() - 1;
    return *this;
}
Environment Environment::operator+(const Definition& def) {
    return Environment(*this) += def;
}

const std::string TURNSTILE = (OnlyAscii ? "|-" : "⊢");

Judgement::Judgement(const Environment& env,
                     const Context& context,
                     const std::shared_ptr<Term>& proof,
                     const std::shared_ptr<Term>& prop)
    : _env(env), _context(context), _term(proof), _type(prop) {}
std::string Judgement::string(bool inSingleLine, size_t indentSize) const {
    std::string res("");
    std::string indent_ex_1(indentSize, '\t');
    std::string indent_ex(inSingleLine ? 0 : indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    res += indent_ex_1 + "Judge<<" + eol;
    res += _env.string(inSingleLine, inSingleLine ? 0 : indentSize + 1);
    res += " ;" + eol + indent_ex + indent_in + _context.string();
    res += " " + TURNSTILE + " " + _term->string();
    res += " : " + _type->string();
    res += eol + indent_ex + ">>";
    return res;
}

std::string Judgement::string_brief(bool inSingleLine, size_t indentSize) const {
    std::string res("");
    std::string indent_ex_1(indentSize, '\t');
    std::string indent_ex(inSingleLine ? 0 : indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    res += indent_ex_1 + "Judge<<" + eol;
    res += _env.string_brief(inSingleLine, inSingleLine ? 0 : indentSize + 1);
    res += " ;" + eol + indent_ex + indent_in + _context.string();
    res += " " + TURNSTILE + " " + _term->string();
    res += " : " + _type->string();
    res += eol + indent_ex + ">>";
    return res;
}

const Environment& Judgement::env() const { return _env; }
const Context& Judgement::context() const { return _context; }
const std::shared_ptr<Term>& Judgement::term() const { return _term; }
const std::shared_ptr<Term>& Judgement::type() const { return _type; }

Environment& Judgement::env() { return _env; }
Context& Judgement::context() { return _context; }
std::shared_ptr<Term>& Judgement::term() { return _term; }
std::shared_ptr<Term>& Judgement::type() { return _type; }

Book::Book() : std::vector<Judgement>{} {};
Book::Book(const std::vector<Judgement>& list) : std::vector<Judgement>(list) {}

// inference rules
void Book::sort() {
    this->emplace_back(
        Environment(),
        Context(),
        star,
        sq);
}
void Book::var(size_t m, char x) {
    check_true_or_exit(
        is_var_applicable(*this, m, x),
        "var at line "
            << this->size() << " not applicable "
            << "(idx = " << m << ", var = " << x << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);

    auto& judge = (*this)[m];
    auto vx = variable(x);
    auto A = judge.term();
    this->emplace_back(
        judge.env(),
        judge.context() + Typed<Variable>(vx, A),
        vx, A);
}
void Book::weak(size_t m, size_t n, char x) {
    check_true_or_exit(
        is_weak_applicable(*this, m, n, x),
        "weak at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ", var = " << x << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto vx = variable(x);
    auto A = judge1.term();
    auto B = judge1.type();
    auto C = judge2.term();
    this->emplace_back(
        judge1.env(),
        judge1.context() + Typed<Variable>(vx, C),
        A, B);
}
void Book::form(size_t m, size_t n) {
    check_true_or_exit(
        is_form_applicable(*this, m, n),
        "form at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto x = judge2.context().back().value();
    auto A = judge1.term();
    auto B = judge2.term();
    auto s2 = judge2.type();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        pi(x, A, B), s2);
}

void Book::appl(size_t m, size_t n) {
    check_true_or_exit(
        is_appl_applicable(*this, m, n),
        "appl at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto M = judge1.term();
    auto N = judge2.term();
    auto B = pi(judge1.type())->expr();
    auto x = pi(judge1.type())->var().value();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        ::appl(M, N),
        substitute(B, x, N));
}

void Book::abst(size_t m, size_t n) {
    check_true_or_exit(
        is_abst_applicable(*this, m, n),
        "abst at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto M = judge1.term();
    auto x = judge1.context().back().value();
    auto A = judge1.context().back().type();
    auto B = judge1.type();
    this->emplace_back(
        judge2.env(),
        judge2.context(),
        lambda(x, A, M),
        pi(x, A, B));
}

void Book::conv(size_t m, size_t n) {
    check_true_or_exit(
        is_conv_applicable(*this, m, n),
        "conv at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto A = judge1.term();
    auto B2 = judge2.term();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        A, B2);
}

void Book::def(size_t m, size_t n, const std::string& a) {
    check_true_or_exit(
        is_def_applicable(*this, m, n, a),
        "def at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ", name = " << a << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto K = judge1.term();
    auto L = judge1.type();
    auto M = judge2.term();
    auto N = judge2.type();
    auto& xAs = judge2.context();
    std::vector<std::shared_ptr<Term>> xs;
    for (auto&& xA : xAs) xs.push_back(xA.value());
    this->emplace_back(
        judge1.env() + Definition(xAs, constant(a, xs), M, N),
        judge1.context(),
        K, L);
}

void Book::defpr(size_t m, size_t n, const std::string& a) {
    check_true_or_exit(
        is_def_applicable(*this, m, n, a),
        "defpr at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ", name = " << a << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
    auto K = judge1.term();
    auto L = judge1.type();
    auto N = judge2.term();
    auto& xAs = judge2.context();
    std::vector<std::shared_ptr<Term>> xs;
    for (auto&& xA : xAs) xs.push_back(xA.value());
    this->emplace_back(
        judge1.env() + Definition(xAs, constant(a, xs), N),
        judge1.context(),
        K, L);
}

void Book::inst(size_t m, size_t n, const std::vector<size_t>& k, size_t p) {
    check_true_or_exit(
        is_inst_applicable(*this, m, n, k, p),
        "inst at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", n = " << n << ", k = " << to_string(k) << ", p = " << p << ")" << std::endl
            << "final state of book:" << std::endl
            << *this,
        __FILE__, __LINE__, __func__);
    auto& judge = (*this)[m];
    auto& D = judge.env()[p];

    auto N = D.type();

    std::vector<std::shared_ptr<Variable>> xs;
    std::vector<std::shared_ptr<Term>> Us;
    for (size_t i = 0; i < n; ++i) {
        xs.push_back(D.context()[i].value());
        Us.push_back((*this)[k[i]].term());
    }

    this->emplace_back(
        judge.env(),
        judge.context(),
        constant(D.definiendum(), Us),
        substitute(N, xs, Us));
}

void Book::cp(size_t m) {
    this->emplace_back((*this)[m]);
}

void Book::sp(size_t m, size_t n) {
    auto& judge = (*this)[m];
    auto& tv = judge.context()[n];
    this->emplace_back(
        judge.env(),
        judge.context(),
        tv.value(),
        tv.type());
}

std::string Book::string() const {
    std::string res("Book[[");
    bool singleLine = true;
    int indentSize = 1;
    if (this->size() > 0) res += "\n[0]" + (*this)[0].string_brief(singleLine, indentSize);
    for (size_t i = 1; i < this->size(); ++i) res += ",\n[" + std::to_string(i) + "]" + (*this)[i].string_brief(singleLine, indentSize);
    res += "\n]]";
    return res;
}
std::string Book::repr() const {
    std::stringstream ss;
    // defs enum
    for (size_t dno = 0; dno < this->env().size(); ++dno) {
        ss << "D" << dno << " : " << this->env()[dno].repr_book() << std::endl;
    }
    if (this->env().size() > 0) ss << "--------------" << std::endl;
    for (size_t lno = 0; lno < this->size(); ++lno) {
        auto& judge = (*this)[lno];
        ss << lno << " : ";
        if (this->env().size() > 0) {
            if (judge.env().size() > 0) ss << "D" << def_num(judge.env()[0]);
            for (size_t dno = 1; dno < judge.env().size(); ++dno) {
                ss << ",D" << def_num(judge.env()[dno]);
            }
        } else if (judge.env().size() > 0) {
            ss << "env(#defs = " << judge.env().size() << ")";
        }
        ss << " ; ";
        // output context
        ss << judge.context().repr_book() << " |- ";
        ss << judge.term()->repr_book() << " : ";
        ss << judge.type()->repr_book() << "\n";
    }
    return ss.str();
}
std::string Book::repr_new() const {
    std::stringstream ss;
    // defs enum
    for (size_t dno = 0; dno < this->env().size(); ++dno) {
        ss << "D" << dno << " : " << this->env()[dno].repr_book() << std::endl;
    }
    if (this->env().size() > 0) ss << "--------------" << std::endl;
    for (size_t lno = 0; lno < this->size(); ++lno) {
        auto& judge = (*this)[lno];
        ss << lno << " : ";
        if (this->env().size() > 0) {
            if (judge.env().size() > 0) ss << "D" << def_num(judge.env()[0]);
            for (size_t dno = 1; dno < judge.env().size(); ++dno) {
                ss << ",D" << def_num(judge.env()[dno]);
            }
        } else if (judge.env().size() > 0) {
            ss << "env(#defs = " << judge.env().size() << ")";
        }
        ss << " ; ";
        // output context
        ss << judge.context().repr_book() << " |- ";
        ss << judge.term()->repr_new() << " : ";
        ss << judge.type()->repr_new() << "\n";
    }
    return ss.str();
}

void Book::read_def_file(const std::string& fname) {
    this->_env = Environment(fname);
    for (size_t dno = 0; dno < this->env().size(); ++dno) {
        this->_def_dict[this->env()[dno].definiendum()] = dno;
    }
}

const Environment& Book::env() const { return _env; }
int Book::def_num(const Definition& def) const { return _def_dict.at(def.definiendum()); }
