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
            std::cerr << "to_string(): not implemented (type:" << (int)k << ")" << std::endl;
            exit(EXIT_FAILURE);
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
            std::vector<std::shared_ptr<Term>> types;
            for (auto&& ptr : t->types()) types.emplace_back(copy(ptr));
            return constant(t->name(), types);
        }
        default:
            std::cerr << "copy(): unknown kind: " << to_string(term->kind()) << std::endl;
            exit(EXIT_FAILURE);
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
            for (auto& type : t->types()) set_union_inplace(FV, free_var(type));
            return FV;
        }
        default:
            std::cerr << "free_var(): unknown kind: " << to_string(term->kind()) << std::endl;
            exit(EXIT_FAILURE);
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
    std::cerr << "out of fresh variable" << std::endl;
    exit(EXIT_FAILURE);
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
            auto z = get_fresh_var(t, expr);
            return lambda(
                z,
                substitute(t->var().type(), bind, expr),
                substitute(substitute(t->expr(), t->var().value(), z), bind, expr));
        }
        case Kind::AbstPi: {
            auto t = pi(term);
            auto z = get_fresh_var(t, expr);
            return pi(
                z,
                substitute(t->var().type(), bind, expr),
                substitute(substitute(t->expr(), t->var().value(), z), bind, expr));
        }
        case Kind::Constant: {
            auto t = constant(term);
            std::vector<std::shared_ptr<Term>> types;
            for (auto& type : t->types()) types.emplace_back(substitute(type, bind, expr));
            return constant(t->name(), types);
        }
        default:
            std::cerr << "substitute(): unknown kind: " << to_string(term->kind()) << std::endl;
            exit(EXIT_FAILURE);
    }
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Term>& var_bind, const std::shared_ptr<Term>& expr) {
    if (var_bind->kind() != Kind::Variable) {
        std::cerr << "substitute(): var_bind kind error (expected Kind::Variable, got " << to_string(var_bind->kind()) << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    return substitute(term, variable(var_bind), expr);
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
    std::vector<std::shared_ptr<Term>> types;
    for (auto& type : ts) types.emplace_back(copy(type));
    return std::make_shared<Constant>(name, types);
}
std::shared_ptr<Constant> constant(const std::shared_ptr<Term>& t){
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
            if (la->types().size() != lb->types().size()) return false;
            for (size_t idx = 0; idx < la->types().size(); ++idx) {
                if (!alpha_comp(la->types()[idx], lb->types()[idx])) return false;
            }
            return true;
        }
        default:
            std::cerr << "alpha_comp(): unknown kind: " << to_string(a->kind()) << std::endl;
            exit(EXIT_FAILURE);
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
            if (la->types().size() != lb->types().size()) return false;
            for (size_t idx = 0; idx < la->types().size(); ++idx) {
                if (!exact_comp(la->types()[idx], lb->types()[idx])) return false;
            }
            return true;
        }
        default:
            std::cerr << "exact_comp(): unknown kind: " << to_string(a->kind()) << std::endl;
            exit(EXIT_FAILURE);
    }
}

bool equiv_context_n(const Context& a, const Context& b, size_t n) {
    check_true(n <= a.data().size());
    check_true(n <= b.data().size());
    for (size_t i = 0; i < n; ++i) {
        check_true(alpha_comp(a.data()[i].value(), b.data()[i].value()));
        check_true(alpha_comp(a.data()[i].type(), b.data()[i].type()));
    }
    return true;
}

bool equiv_context_n(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b, size_t n) {
    return equiv_context_n(*a, *b, n);
}

bool equiv_context(const Context& a, const Context& b) {
    check_true(a.data().size() == b.data().size());
    return equiv_context_n(a, b, a.data().size());
}

bool equiv_context(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b) {
    return equiv_context(*a, *b);
}

bool equiv_def(const Definition& a, const Definition& b) {
    check_true(equiv_context(a.context(), b.context()));
    check_true(alpha_comp(a.definiendum(), b.definiendum()));
    check_true(a.is_prim() != b.is_prim());
    check_true(a.is_prim() || alpha_comp(a.definiens(), b.definiens()));
    check_true(alpha_comp(a.type(), b.type()));
    return true;
}

bool equiv_def(const std::shared_ptr<Definition>& a, const std::shared_ptr<Definition>& b) {
    return equiv_def(*a, *b);
}

bool equiv_env(const Environment& a, const Environment& b) {
    check_true(a.defs().size() == b.defs().size());
    for (size_t i = 0; i < a.defs().size(); ++i) {
        check_true(equiv_def(a.defs()[i], b.defs()[i]));
    }
    return true;
}

bool equiv_env(const std::shared_ptr<Environment>& a, const std::shared_ptr<Environment>& b) {
    return equiv_env(*a, *b);
}

bool has_variable(const Context& g, const std::shared_ptr<Variable>& v) {
    for (auto& tv : g.data()) {
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
    for(auto& def: env.defs()) {
        if (def.definiendum()->name() == name) return true;
    }
    return false;
}

bool is_beta_reachable(const std::shared_ptr<Term>& from, const std::shared_ptr<Term>& to) {
    unused(from, to);
    std::cerr << "is_beta_reachable not implemented" << std::endl;
    exit(EXIT_FAILURE);
}

bool is_sort(const std::shared_ptr<Term>& t) {
    return t->kind() == Kind::Star || t->kind() == Kind::Square;
}

bool is_var_applicable(const Book& book, size_t idx, char var) {
    auto& judge = book[idx];
    check_true(is_sort(judge.type()));
    check_false(has_variable(judge.context(), var));
    return true;
}

bool is_var_applicable(const std::shared_ptr<Book>& book, size_t idx, char var) {
    return is_var_applicable(*book, idx, var);
}

bool is_weak_applicable(const Book& book, size_t idx1, size_t idx2, char var){
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_true(equiv_context(judge1.context(), judge2.context()));
    check_true(is_sort(judge2.type()));
    check_false(has_variable(judge1.context(), var));
    return true;
}

bool is_weak_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2, char var) {
    return is_weak_applicable(*book, idx1, idx2, var);
}

bool is_form_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_true(equiv_context_n(judge1.context(), judge2.context(), judge1.context().size()));
    check_true(judge1.context().size() + 1 == judge2.context().size());
    check_true(alpha_comp(judge1.term(), judge2.context().back().type()));
    check_true(is_sort(judge1.type()));
    check_true(is_sort(judge2.type()));
    return true;
}

bool is_form_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2) {
    return is_form_applicable(*book, idx1, idx2);
}

bool is_appl_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_true(equiv_context(judge1.context(), judge2.context()));
    check_true(judge1.type()->kind() == Kind::AbstPi);
    auto p = pi(judge1.type());
    check_true(alpha_comp(p->var().type(), judge2.type()));
    return true;
}

bool is_appl_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2) {
    return is_appl_applicable(*book, idx1, idx2);
}

bool is_abst_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_true(equiv_context_n(judge1.context(), judge2.context(), judge2.context().size()));
    check_true(judge1.context().size() == judge2.context().size() + 1);
    check_true(judge2.term()->kind() == Kind::AbstPi);
    auto p = pi(judge2.term());
    auto x = judge1.context().back().value();
    auto A = judge1.context().back().type();
    auto B = judge1.type();
    check_true(alpha_comp(p->var().value(), x));
    check_true(alpha_comp(p->var().type(), A));
    check_true(alpha_comp(p->expr(), B));
    check_true(is_sort(judge2.type()));
    return true;
}

bool is_abst_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2) {
    return is_abst_applicable(*book, idx1, idx2);
}

bool is_conv_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    unused(judge1, judge2);
    std::cerr << "is_conv not implemented" << std::endl;
    exit(EXIT_FAILURE);
}

bool is_conv_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2) {
    return is_conv_applicable(*book, idx1, idx2);
}

bool is_def_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    unused(judge1, judge2, name);
    std::cerr << "is_def not implemented" << std::endl;
    exit(EXIT_FAILURE);
}

bool is_def_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2, const std::string& name) {
    return is_def_applicable(*book, idx1, idx2, name);
}

bool is_def_prim_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    unused(judge1, judge2, name);
    std::cerr << "is_def_prim not implemented" << std::endl;
    exit(EXIT_FAILURE);
}

bool is_def_prim_applicable(const std::shared_ptr<Book>& book, size_t idx1, size_t idx2, const std::string& name) {
    return is_def_prim_applicable(*book, idx1, idx2, name);
}
