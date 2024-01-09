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
        case Kind::Test01:
            return "Kind::Test01";
        case Kind::Test02:
            return "Kind::Test02";
        case Kind::Test03:
            return "Kind::Test03";
        default:
            return "[to_string(const Kind&): TBI]";
    }
}

std::ostream& operator<<(std::ostream& os, const Kind& k) {
    return os << to_string(k);
}

std::shared_ptr<Term> copy(const std::shared_ptr<Term>& term) {
    switch (term->kind()) {
        case Kind::Star:
            return std::make_shared<Star>();
        case Kind::Square:
            return std::make_shared<Square>();
        case Kind::Variable: {
            auto t = std::dynamic_pointer_cast<Variable>(term);
            return std::make_shared<Variable>(t->name());
        }
        case Kind::Application: {
            auto t = std::dynamic_pointer_cast<Application>(term);
            return std::make_shared<Application>(copy(t->M()), copy(t->N()));
        }
        case Kind::AbstLambda: {
            auto t = std::dynamic_pointer_cast<AbstLambda>(term);
            return std::make_shared<AbstLambda>(copy(t->var()->value()), copy(t->var()->type()), copy(t->expr()));
        }
        case Kind::AbstPi: {
            auto t = std::dynamic_pointer_cast<AbstPi>(term);
            return std::make_shared<AbstPi>(copy(t->var()->value()), copy(t->var()->type()), copy(t->expr()));
        }
        case Kind::Constant: {
            auto t = std::dynamic_pointer_cast<Constant>(term);
            std::vector<std::shared_ptr<Term>> types;
            for (auto&& ptr : t->types()) types.emplace_back(copy(ptr));
            return std::make_shared<Constant>(t->name(), types);
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
            auto t = std::dynamic_pointer_cast<Variable>(term);
            FV.insert(t->name());
            return FV;
        }
        case Kind::Application: {
            auto t = std::dynamic_pointer_cast<Application>(term);
            set_union(free_var(t->M()), free_var(t->N()), FV);
            return FV;
        }
        case Kind::AbstLambda: {
            auto t = std::dynamic_pointer_cast<AbstLambda>(term);
            FV = free_var(t->expr());
            FV.erase(t->var()->value()->name());
            return FV;
        }
        case Kind::AbstPi: {
            auto t = std::dynamic_pointer_cast<AbstPi>(term);
            FV = free_var(t->expr());
            FV.erase(t->var()->value()->name());
            return FV;
        }
        case Kind::Constant: {
            auto t = std::dynamic_pointer_cast<Constant>(term);
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
    if (!univ.empty()) return std::make_shared<Variable>(*univ.begin());
    std::cerr << "out of fresh variable" << std::endl;
    exit(EXIT_FAILURE);
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Variable>& bind, const std::shared_ptr<Term>& expr) {
    switch (term->kind()) {
        case Kind::Star:
        case Kind::Square:
            return copy(term);
        case Kind::Variable:
            return copy(alpha_comp(term, bind) ? expr : term);
        case Kind::Application: {
            auto t = std::dynamic_pointer_cast<Application>(term);
            return std::make_shared<Application>(
                substitute(t->M(), bind, expr),
                substitute(t->N(), bind, expr));
        }
        case Kind::AbstLambda: {
            auto t = std::dynamic_pointer_cast<AbstLambda>(term);
            auto z = get_fresh_var(t, expr);
            return std::make_shared<AbstLambda>(
                z,
                substitute(t->var()->type(), bind, expr),
                substitute(substitute(t->expr(), t->var()->value(), z), bind, expr));
        }
        case Kind::AbstPi: {
            auto t = std::dynamic_pointer_cast<AbstPi>(term);
            auto z = get_fresh_var(t, expr);
            return std::make_shared<AbstPi>(
                z,
                substitute(t->var()->type(), bind, expr),
                substitute(substitute(t->expr(), t->var()->value(), z), bind, expr));
        }
        case Kind::Constant: {
            auto t = std::dynamic_pointer_cast<Constant>(term);
            std::vector<std::shared_ptr<Term>> types;
            for (auto& type : t->types()) types.emplace_back(substitute(type, bind, expr));
            return std::make_shared<Constant>(t->name(), types);
        }
        default:
            std::cerr << "substitute(): unknown kind: " << to_string(term->kind()) << std::endl;
            exit(EXIT_FAILURE);
    }
}

std::shared_ptr<Term> substitute(const std::shared_ptr<Term>& term, const std::shared_ptr<Term>& var_bind, const std::shared_ptr<Term>& expr){
    if (var_bind->kind() != Kind::Variable) {
        std::cerr << "substitute(): var_bind kind error (expected Kind::Variable, got " << to_string(var_bind->kind()) << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    return substitute(term, std::dynamic_pointer_cast<Variable>(var_bind), expr);
}

bool alpha_comp(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b) {
    if (a->kind() != b->kind()) return false;
    switch (a->kind()) {
        case Kind::Star:
        case Kind::Square:
            return true;
        case Kind::Variable: {
            auto la = std::dynamic_pointer_cast<Variable>(a);
            auto lb = std::dynamic_pointer_cast<Variable>(b);
            return la->name() == lb->name();
        }
        case Kind::AbstLambda: {
            auto la = std::dynamic_pointer_cast<AbstLambda>(a);
            auto lb = std::dynamic_pointer_cast<AbstLambda>(b);
            if (!alpha_comp(la->var()->type(), lb->var()->type())) return false;
            return alpha_comp(la->expr(), substitute(lb->expr(), lb->var()->value(), la->var()->value()));
        }
        case Kind::AbstPi: {
            auto la = std::dynamic_pointer_cast<AbstPi>(a);
            auto lb = std::dynamic_pointer_cast<AbstPi>(b);
            if (!alpha_comp(la->var()->type(), lb->var()->type())) return false;
            return alpha_comp(la->expr(), substitute(lb->expr(), lb->var()->value(), la->var()->value()));
        }
        case Kind::Application: {
            auto la = std::dynamic_pointer_cast<Application>(a);
            auto lb = std::dynamic_pointer_cast<Application>(b);
            return alpha_comp(la->M(), lb->M()) && alpha_comp(la->N(), lb->N());
        }
        case Kind::Constant: {
            auto la = std::dynamic_pointer_cast<Constant>(a);
            auto lb = std::dynamic_pointer_cast<Constant>(b);
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
            auto la = std::dynamic_pointer_cast<Variable>(a);
            auto lb = std::dynamic_pointer_cast<Variable>(b);
            return la->name() == lb->name();
        }
        case Kind::AbstLambda: {
            auto la = std::dynamic_pointer_cast<AbstLambda>(a);
            auto lb = std::dynamic_pointer_cast<AbstLambda>(b);
            return exact_comp(la->var()->value(), lb->var()->value()) && exact_comp(la->var()->type(), lb->var()->type()) && exact_comp(la->expr(), lb->expr());
        }
        case Kind::AbstPi: {
            auto la = std::dynamic_pointer_cast<AbstPi>(a);
            auto lb = std::dynamic_pointer_cast<AbstPi>(b);
            return exact_comp(la->var()->value(), lb->var()->value()) && exact_comp(la->var()->type(), lb->var()->type()) && exact_comp(la->expr(), lb->expr());
        }
        case Kind::Application: {
            auto la = std::dynamic_pointer_cast<Application>(a);
            auto lb = std::dynamic_pointer_cast<Application>(b);
            return exact_comp(la->M(), lb->M()) && exact_comp(la->N(), lb->N());
        }
        case Kind::Constant: {
            auto la = std::dynamic_pointer_cast<Constant>(a);
            auto lb = std::dynamic_pointer_cast<Constant>(b);
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
