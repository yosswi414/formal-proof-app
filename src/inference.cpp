#include "inference.hpp"

TypeError::TypeError(const std::string& str, const std::shared_ptr<Term>& term, const std::shared_ptr<Context>& con) : msg(str), term(term), con(con){}

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
