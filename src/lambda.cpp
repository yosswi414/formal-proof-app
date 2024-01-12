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

std::shared_ptr<Variable> get_fresh_var(const std::vector<std::shared_ptr<Term>>& terms) {
    std::set<char> univ;
    for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    for (auto&& t : terms) set_minus_inplace(univ, free_var(t));
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

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::vector<std::shared_ptr<Variable>>& vars, const std::vector<std::shared_ptr<Term>>& exprs){
    if (vars.size() != exprs.size()) {
        std::cerr << "substitute(): length of vars and exprs doesn't match" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (vars.size() == 0) return term;
    std::vector<std::shared_ptr<Term>> used;
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
    check_true(n <= a.size());
    check_true(n <= b.size());
    for (size_t i = 0; i < n; ++i) {
        check_true(alpha_comp(a[i].value(), b[i].value()));
        check_true(alpha_comp(a[i].type(), b[i].type()));
    }
    return true;
}

bool equiv_context_n(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b, size_t n) {
    return equiv_context_n(*a, *b, n);
}

bool equiv_context(const Context& a, const Context& b) {
    check_true(a.size() == b.size());
    return equiv_context_n(a, b, a.size());
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
    check_true(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        check_true(equiv_def(a[i], b[i]));
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
        if (def.definiendum()->name() == name) return true;
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
    check_true(is_sort(judge.type()));
    check_false(has_variable(judge.context(), var));
    return true;
}

bool is_weak_applicable(const Book& book, size_t idx1, size_t idx2, char var) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_true(equiv_context(judge1.context(), judge2.context()));
    check_true(is_sort(judge2.type()));
    check_false(has_variable(judge1.context(), var));
    return true;
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

std::shared_ptr<Term> beta_reduce(const std::shared_ptr<Term>& term) {

}

bool is_beta_reachable(const std::shared_ptr<Term>& from, const std::shared_ptr<Term>& to) {
    unused(from, to);
    std::cerr << "is_beta_reachable not implemented" << std::endl;
    exit(EXIT_FAILURE);
}

bool is_conv_applicable(const Book& book, size_t idx1, size_t idx2) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_true(equiv_context(judge1.context(), judge2.context()));
    auto B1 = judge1.type();
    auto B2 = judge2.term();
    auto s = judge2.type();
    check_true(is_beta_reachable(B1, B2));
    check_true(is_sort(s));
    return true;
}

bool is_def_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_false(has_constant(judge1.env(), name));;
    return true;
}

bool is_def_prim_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    auto& judge1 = book[idx1];
    auto& judge2 = book[idx2];
    check_true(equiv_env(judge1.env(), judge2.env()));
    check_false(has_constant(judge1.env(), name));
    check_true(is_sort(judge2.type()));
    return true;
}

bool is_inst_applicable(const Book& book, size_t idx, size_t n, const std::vector<size_t>& k, size_t p) {
    auto& judge = book[idx];

    check_true(k.size() == n);
    for (size_t i = 0; i < n; ++ i) {
        check_true(equiv_env(judge.env(), book[k[i]].env()));
        check_true(equiv_context(judge.context(), book[k[i]].context()));
    }

    auto& D = judge.env()[p];
    check_true(D.definiendum()->types().size() == n);

    std::vector<std::shared_ptr<Term>> Us;
    std::vector<std::shared_ptr<Variable>> xs;
    for (size_t i = 0; i < n; ++i) {
        auto A = D.context()[i].type();
        auto V = book[k[i]].type();
        // check V == A[xs := Us]
        check_true(alpha_comp(V, substitute(A, xs, Us)));
        xs.push_back(D.context()[i].value());
        Us.push_back(book[k[i]].term());
    }

    check_true(judge.term()->kind() == Kind::Star);
    check_true(judge.type()->kind() == Kind::Square);

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

Constant::Constant(const std::string& name, std::vector<std::shared_ptr<Term>> list) : Term(Kind::Constant), _name(name), _types(list) {}

const std::vector<std::shared_ptr<Term>>& Constant::types() const { return _types; }
std::vector<std::shared_ptr<Term>>& Constant::types() { return _types; }
const std::string& Constant::name() const { return _name; }
std::string& Constant::name() { return _name; }

std::string Constant::string() const {
    std::string res(_name);
    res += "[";
    if (_types.size() > 0) res += _types[0]->string();
    for (size_t i = 1; i < _types.size(); ++i) res += ", " + _types[i]->string();
    res += "]";
    return res;
}
std::string Constant::repr() const {
    std::string res(_name);
    res += "[";
    if (_types.size() > 0) res += "(" + _types[0]->repr() + ")";
    for (size_t i = 1; i < _types.size(); ++i) res += ",(" + _types[i]->repr() + ")";
    res += "]";
    return res;
}
std::string Constant::repr_new() const {
    std::string res(_name);
    res += "[";
    if (_types.size() > 0) res += _types[0]->repr_new();
    for (size_t i = 1; i < _types.size(); ++i) res += ", " + _types[i]->repr_new();
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
    if (this->size() > 0) ss << (*this)[0].value()->repr() << ":" << (*this)[0].type()->repr();
    for (size_t i = 1; i < this->size(); ++i) ss << ", " << (*this)[i].value()->repr() << ":" << (*this)[i].type()->repr();
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
                       const std::shared_ptr<Constant>& constant,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(constant),
      _definiens(nullptr),
      _type(prop) {}

Definition::Definition(const Context& context,
                       const std::shared_ptr<Constant>& constant,
                       const std::shared_ptr<Term>& proof,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(constant),
      _definiens(proof),
      _type(prop) {}

std::string Definition::string() const {
    std::string res;
    res = (_definiens ? "Def< " : "Def-prim< ");
    res += _context.string();
    res += " " + DEFINITION_SEPARATOR + " " + _definiendum->name();
    res += " := " + (_definiens ? _definiens->string() : EMPTY_DEFINIENS);
    res += " : " + _type->string();
    res += " >";
    return res;
}
std::string Definition::repr() const {
    std::string res;
    res = "def2\n";
    res += _context.repr();
    res += _definiendum->name() + "\n";
    res += (_definiens ? _definiens->repr() : "#") + "\n";
    res += _type->repr() + "\n";
    res += "edef2\n";
    return res;
}

std::string Definition::repr_new() const {
    std::string res;
    res = "def2\n";
    res += _context.repr_new();
    res += _definiendum->name() + " := " + (_definiens ? _definiens->repr_new() : "#") + " : " + _type->repr_new() + "\n";
    res += "edef2\n";
    return res;
}

bool Definition::is_prim() const { return !_definiens; }
const Context& Definition::context() const { return _context; }
const std::shared_ptr<Constant>& Definition::definiendum() const { return _definiendum; }
const std::shared_ptr<Term>& Definition::definiens() const { return _definiens; }
const std::shared_ptr<Term>& Definition::type() const { return _type; }

Context& Definition::context() { return _context; }
std::shared_ptr<Constant>& Definition::definiendum() { return _definiendum; }
std::shared_ptr<Term>& Definition::definiens() { return _definiens; }
std::shared_ptr<Term>& Definition::type() { return _type; }

const std::string HEADER_ENV = (OnlyAscii ? "Env" : "Δ");

Environment::Environment() {}
Environment::Environment(const std::vector<Definition>& defs) : std::vector<Definition>(defs) {}
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
std::string Environment::repr_book() const {
    // to be implemented
    return "";
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
    if (!is_var_applicable(*this, m, x)) {
        std::cerr << "var not applicable ";
        std::cerr << "(idx = " << m << ", var = " << x << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto& judge = (*this)[m];
    auto vx = variable(x);
    auto A = judge.term();
    this->emplace_back(
        judge.env(),
        judge.context() + Typed<Variable>(vx, A),
        vx, A);
}
void Book::weak(size_t m, size_t n, char x) {
    if (!is_weak_applicable(*this, m, n, x)) {
        std::cerr << "weak not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ", var = " << x << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
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
    if (!is_form_applicable(*this, m, n)) {
        std::cerr << "form not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
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

void Book::appl(size_t m, size_t n){
    if (!is_appl_applicable(*this, m, n)) {
        std::cerr << "appl not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
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
    if (!is_abst_applicable(*this, m, n)) {
        std::cerr << "abst not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto& judge1 = (*this)[m];
    auto M = judge1.term();
    auto x = judge1.context().back().value();
    auto A = judge1.context().back().type();
    auto B = judge1.type();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        lambda(x, A, M),
        pi(x, A, B));
}

void Book::conv(size_t m, size_t n) {
    if (!is_conv_applicable(*this, m, n)) {
        std::cerr << "conv not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
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
    if (!is_def_applicable(*this, m, n, a)) {
        std::cerr << "def not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ", name = " << a << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
}

void Book::defpr(size_t m, size_t n, const std::string& a) {
    if (!is_def_applicable(*this, m, n, a)) {
        std::cerr << "defpr not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ", name = " << a << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
}

void Book::inst(size_t m, size_t n, const std::vector<size_t>& k, size_t p) {
    if (!is_inst_applicable(*this, m, n, k, p)) {
        std::cerr << "def not applicable ";
        std::cerr << "(idx1 = " << m << ", idx2 = " << n << ", p = " << p << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto& judge1 = (*this)[m];
    auto& judge2 = (*this)[n];
}

void Book::cp(size_t m) {
    this->emplace_back((*this)[m]);
}

void Book::sp(size_t m, size_t n){
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
    if (this->size() > 0) res += "\n" + (*this)[0].string(singleLine, indentSize);
    for (size_t i = 1; i < this->size(); ++i) res += ",\n" + (*this)[i].string(singleLine, indentSize);
    res += "\n]]";
    return res;
}
std::string Book::repr() const {
    std::stringstream ss;
    for (size_t lno = 0; lno < this->size(); ++lno) {
        auto& judge = (*this)[lno];
        ss << lno << " : ";
        // output environment (to be impl'ed)
        ss << judge.env().repr_book() << " ; ";
        // output context
        ss << judge.context().repr_book() << " |- ";
        ss << judge.term()->repr_book() << " : ";
        ss << judge.type()->repr_book() << "\n";
    }
    return ss.str();
}
std::string Book::repr_new() const {
    std::stringstream ss;
    for (size_t lno = 0; lno < this->size(); ++lno) {
        auto& judge = (*this)[lno];
        ss << lno << " : ";
        // output environment (to be impl'ed)
        ss << judge.env().repr_book() << " ; ";
        // output context
        ss << judge.context().repr_book() << " |- ";
        ss << judge.term()->repr_new() << " : ";
        ss << judge.type()->repr_new() << "\n";
    }
    return ss.str();
}
