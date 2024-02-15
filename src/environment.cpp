#include "environment.hpp"

#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "parser.hpp"

Environment::Environment() {}
Environment::Environment(const std::vector<std::shared_ptr<Definition>>& defs) : std::vector<std::shared_ptr<Definition>>(defs) {
    for (size_t idx = 0; idx < this->size(); ++idx) {
        _def_index[(*this)[idx]->definiendum()] = idx;
    }
}

std::vector<std::shared_ptr<FileData>> raw_fname_fds;

Environment::Environment(const std::string& fname) {
    std::shared_ptr<FileData> fdp = std::make_shared<FileData>(fname);
    raw_fname_fds.push_back(fdp);
    auto tokens = tokenize(*fdp);
    *this = parse_defs(tokens);
}

std::string Environment::string(bool inSingleLine, size_t indentSize) const {
    std::string res = "";
    std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    if (this->size() == 0) return indent_ex + SYMBOL_EMPTY;
    res += indent_ex + HEADER_ENV + "{{" + eol;
    if (this->size() > 0) res += indent_ex + indent_in + (*this)[0]->string();
    for (size_t i = 1; i < this->size(); ++i) res += "," + eol + indent_ex + indent_in + (*this)[i]->string();
    res += eol + indent_ex + "}}";
    return res;
}

std::string Environment::string_brief(bool inSingleLine, size_t indentSize) const {
    std::string res = "";
    std::string indent_ex(indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    if (this->size() == 0) return indent_ex + SYMBOL_EMPTY;
    res += indent_ex + HEADER_ENV + "{{" + eol;
    if (this->size() > 0) res += indent_ex + indent_in + (*this)[0]->definiendum();
    for (size_t i = 1; i < this->size(); ++i) res += "," + eol + indent_ex + indent_in + (*this)[i]->definiendum();
    res += eol + indent_ex + "}}";
    return res;
}

std::string Environment::repr() const {
    std::string res = "";
    for (auto&& def : *this) {
        Context& con_old = *def->context();
        Context con_new = con_old;
        auto cname = def->definiendum();
        auto term = def->definiens();
        auto type = def->type();
        for (size_t i = 0; i < con_new.size(); ++i) {
            if (con_new[i].value()->name().size() == 1) continue;
            auto var_old = con_new[i].value();
            auto var_new = get_fresh_var(con_new);
            // std::cerr << "[debug @ Environment::repr() / substitution] var = " << var_old << " -> " << var_new << "\n";
            con_new[i].value() = var_new;
            // std::cerr << "\tcontext subst (j = -): old cont = " << con_new << std::endl;
            for (size_t j = i + 1; j < con_new.size(); ++j) {
                // std::cerr << "\tcontext subst (j = " << j << "): old cont = " << con_new << std::endl;
                std::shared_ptr<Term> con_type = con_new[j].type();
                // std::cerr << "\tcontext subst (j = " << j << "): old type = " << con_type << "\n";
                con_type = substitute(con_type, var_old, var_new);
                // std::cerr << "\tcontext subst (j = " << j << "): new type = " << con_type << "\n";
                con_new[j].type() = con_type;
                // std::cerr << "\tcontext subst (j = " << j << "): new cont = " << con_new << std::endl;
            }
            if (term){
                // std::cerr << "\told term = " << term << "\n";
                term = substitute(term, var_old, var_new);
                // std::cerr << "\tnew term = " << term << "\n";
            }
            // std::cerr << "\told type = " << type << "\n";
            type = substitute(type, var_old, var_new);
            // std::cerr << "\tnew type = " << type << std::endl;
        }
        for (size_t i = 0; i < con_new.size(); ++i) {
            con_new[i].type() = rename_var_short(con_new[i].type());
        }
        if (term) term = rename_var_short(term);
        type = rename_var_short(type);
        // for(auto&& tv : con_old) {
        //     if (tv.value()->name().size() > 1) {
        //         auto var_old = tv.value();
        //         auto var_new = get_fresh_var(con_old);
        //         std::cerr << "[debug @ Environment::repr() / substitution] var = " << var_old << " -> " << var_new << "\n";
        //         con_new.emplace_back(var_new, tv.type());
        //         std::cerr << "\told term = " << term << "\n";
        //         term = substitute(term, var_old, var_new);
        //         std::cerr << "\tnew term = " << term << "\n";
        //         std::cerr << "\told type = " << type << "\n";
        //         type = substitute(type, var_old, var_new);
        //         std::cerr << "\tnew type = " << type << std::endl;
        //         con_new.emplace_back(var_new, )
        //     } else con_new.push_back(tv);
        // }
        // std::cerr << "[debug @ Environment::repr() / Definition generation] def = \n";
        auto newdef = Definition(std::make_shared<Context>(con_new), cname, term, type);
        // std::cerr << newdef.repr() << std::endl;
        res += newdef.repr() + "\n";
        // res += def->repr() + "\n";
    }
    res += "END\n";
    return res;
}
std::string Environment::repr_new() const {
    std::string res = "";
    for (auto&& def : *this) res += def->repr_new() + "\n";
    res += "END\n";
    return res;
}

int Environment::lookup_index(const std::string& cname) const {
    auto itr = _def_index.find(cname);
    if (itr != _def_index.end()) return itr->second;
    for (size_t idx = 0; idx < this->size(); ++idx) {
        if ((*this)[idx]->definiendum() == cname) {
            return _def_index[cname] = idx;
        }
    }
    return -1;
}
int Environment::lookup_index(const std::shared_ptr<Constant>& c) const {
    return lookup_index(c->name());
}

const std::shared_ptr<Definition> Environment::lookup_def(const std::string& cname) const {
    int idx = lookup_index(cname);
    return idx < 0 ? nullptr : (*this)[idx];
}
const std::shared_ptr<Definition> Environment::lookup_def(const std::shared_ptr<Constant>& c) const {
    return lookup_def(c->name());
}

Environment& Environment::operator+=(const std::shared_ptr<Definition>& def) {
    this->push_back(def);
    _def_index[def->definiendum()] = this->size() - 1;
    return *this;
}
Environment Environment::operator+(const std::shared_ptr<Definition>& def) const {
    return Environment(*this) += def;
}

bool equiv_env(const Environment& a, const Environment& b) {
    check_true_or_ret_false(
        a.size() == b.size(),
        "equiv_env(): # of definitions doesn't match",
        __FILE__, __LINE__, __func__);
    for (size_t i = 0; i < a.size(); ++i) {
        // check_true_or_ret_false(
        //     equiv_def(a[i], b[i]),
        //     "equiv_env(): "
        //         << "the " << i << "-th definition of environment doesn't match" << std::endl
        //         << "def 1: " << a[i] << std::endl
        //         << "def 2: " << b[i],
        //     __FILE__, __LINE__, __func__);
        check_true_or_ret_false(
            a[i]->definiendum() == b[i]->definiendum(),
            "equiv_env(): "
                << "the " << i << "-th definition of environment doesn't match" << std::endl
                << "def 1: " << a[i] << std::endl
                << "def 2: " << b[i],
            __FILE__, __LINE__, __func__);
    }
    return true;
}

bool equiv_env(const std::shared_ptr<Environment>& a, const std::shared_ptr<Environment>& b) {
    if (a == b) return true;
    return equiv_env(*a, *b);
}

bool has_constant(const std::shared_ptr<Environment>& env, const std::string& name) {
    for (auto&& def : *env) {
        if (def->definiendum() == name) return true;
    }
    return false;
}

bool has_definition(const std::shared_ptr<Environment>& env, const std::shared_ptr<Definition>& def) {
    for (auto&& d : *env) {
        if (equiv_def(d, def)) return true;
    }
    return false;
}

std::shared_ptr<Term> delta_reduce(const std::shared_ptr<Constant>& term, const Environment& delta) {
    const std::shared_ptr<Definition> D = delta.lookup_def(term);

    if (D->is_prim()) return term;

    auto M = D->definiens();
    std::vector<std::shared_ptr<Variable>> xs;
    for (size_t i = 0; i < D->context()->size(); ++i) {
        xs.push_back((*D->context())[i].value());
    }

    return substitute(M, xs, term->args());
}

int expr_rank(const std::shared_ptr<Term>& term, const Environment& delta) {
    switch(term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        case EpsilonType::Variable:
            return -1;
        case EpsilonType::Application: {
            auto t = appl(term);
            return std::max(expr_rank(t->M(), delta), expr_rank(t->N(), delta));
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            return std::max(expr_rank(t->var().type(), delta), expr_rank(t->expr(), delta));
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            return std::max(expr_rank(t->var().type(), delta), expr_rank(t->expr(), delta));
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            int rank = delta.lookup_index(t);
            for (auto&& arg : t->args()) rank = std::max(rank, expr_rank(arg, delta));
            return rank;
        }
    }
    return -1;
}

std::shared_ptr<Term> delta_nf_above(const std::shared_ptr<Term>& term, const Environment& delta, int idx) {
    switch (term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        case EpsilonType::Variable:
            return term;
        case EpsilonType::Application: {
            auto t = appl(term);
            return appl(
                delta_nf(t->M(), delta),
                delta_nf(t->N(), delta));
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            return lambda(
                t->var().value(),
                delta_nf(t->var().type(), delta),
                delta_nf(t->expr(), delta));
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            return pi(
                t->var().value(),
                delta_nf(t->var().type(), delta),
                delta_nf(t->expr(), delta));
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            auto rank = delta.lookup_index(t);
            auto dptr = delta.lookup_def(t);
            if (rank < idx || delta[rank]->is_prim()) {
                if (t->args().size() == 0) return t;
                std::vector<std::shared_ptr<Term>> nargs;
                for (auto&& arg : t->args()) nargs.push_back(delta_nf(arg, delta));
                return constant(t->name(), nargs);
            }

            return delta_nf(delta_reduce(t, delta), delta);
        }
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}

std::shared_ptr<Term> delta_nf(const std::shared_ptr<Term>& term, const Environment& delta) {
    return delta_nf_above(term, delta, 0);
}

std::shared_ptr<Term> NF_above(const std::shared_ptr<Term>& term, const Environment& delta, int idx) {
    std::shared_ptr<Term> t, t_prev, t_dx;
    t = term;
    do {
        t_prev = t;
        t = beta_nf(t);
        if (!alpha_comp(t, beta_nf(t))) {
            debug("beta_nf found not to be idempotent. aborting...");
            exit(EXIT_FAILURE);
        }
        t = delta_nf_above(t, delta, idx);
        if (!alpha_comp(t, delta_nf_above(t, delta, idx))) {
            debug("delta_nf found not to be idempotent. aborting...");
            exit(EXIT_FAILURE);
        }
    } while (!alpha_comp(t, t_prev));
    return t;
}

std::shared_ptr<Term> NF(const std::shared_ptr<Term>& term, const Environment& delta) {
    return NF_above(term, delta, 0);
}
std::shared_ptr<Term> NF(const std::shared_ptr<Term>& term, const std::shared_ptr<Environment>& delta) {
    return NF(term, *delta);
}

bool is_delta_reducible(const std::shared_ptr<Term>& term, const Environment& delta) {
    check_true_or_ret_false_nomsg(term->etype() == EpsilonType::Constant);
    std::shared_ptr<Constant> t = constant(term);
    check_true_or_ret_false_nomsg(delta.lookup_index(t) >= 0);
    check_true_or_ret_false_nomsg(!delta.lookup_def(t)->is_prim());
    return true;
}

bool is_constant_defined(const std::string& cname, const Environment& delta) {
    return delta.lookup_index(cname) >= 0;
}
bool is_constant_primitive(const std::string& cname, const Environment& delta) {
    auto ptr = delta.lookup_def(cname);
    return ptr && ptr->is_prim();
}

bool is_normal_form(const std::shared_ptr<Term>& term, const Environment& delta) {
    switch (term->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        case EpsilonType::Variable:
            return true;
        case EpsilonType::Application: {
            auto t = appl(term);
            if (t->M()->etype() == EpsilonType::AbstLambda) return false;
            return is_normal_form(t->M(), delta) && is_normal_form(t->N(), delta);
        }
        case EpsilonType::AbstLambda: {
            auto t = lambda(term);
            return is_normal_form(t->var().type(), delta) && is_normal_form(t->expr(), delta);
        }
        case EpsilonType::AbstPi: {
            auto t = pi(term);
            return is_normal_form(t->var().type(), delta) && is_normal_form(t->expr(), delta);
        }
        case EpsilonType::Constant: {
            auto t = constant(term);
            if (delta.lookup_index(t) < 0 || delta.lookup_def(t)->is_prim()) {
                for (auto&& arg : t->args()) {
                    if (!is_normal_form(arg, delta)) return false;
                }
                return true;
            }
            return false;
        }
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}
std::shared_ptr<Term> reduce_application(const std::shared_ptr<Application>& term, const Environment& delta) {
    auto M = term->M();
    auto N = term->N();
    switch (M->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        case EpsilonType::Variable:
        case EpsilonType::AbstPi:
            return nullptr;
        case EpsilonType::AbstLambda:
            return beta_reduce(term);
        case EpsilonType::Application: {
            std::shared_ptr<Term> rM = nullptr;
            if (rM = reduce_application(appl(M), delta)) return reduce_application(appl(rM, N), delta);
            return nullptr;
        }
        case EpsilonType::Constant: {
            std::shared_ptr<Constant> cM = constant(M);
            if (delta.lookup_index(cM) < 0 || delta.lookup_def(cM)->is_prim()) return nullptr;
            return reduce_application(appl(delta_reduce(cM, delta), N), delta);
        }
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}

bool is_convertible(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b, const Environment& delta) {
    // std::cerr << "conv a = " << a << std::endl;
    // std::cerr << "conv b = " << b << std::endl;
    if (flag_address_comp && a == b) return true;

    if (a->etype() == b->etype()) {
        switch (a->etype()) {
            case EpsilonType::Star:
            case EpsilonType::Square:
            case EpsilonType::Variable:
                return alpha_comp(a, b);
            case EpsilonType::Application: {
                auto aa = appl(a);
                auto ab = appl(b);
                auto M = aa->M();
                auto N = aa->N();
                auto K = ab->M();
                auto L = ab->N();
                if (is_convertible(M, K, delta) &&
                    is_convertible(N, L, delta)) return true;
                std::shared_ptr<Term> ra = nullptr;
                if (ra = reduce_application(aa, delta)) return is_convertible(ra, ab, delta);
                std::shared_ptr<Term> rb = nullptr;
                if (rb = reduce_application(ab, delta)) return is_convertible(aa, rb, delta);
                return false;
            }
            case EpsilonType::AbstLambda: {
                auto la = lambda(a);
                auto lb = lambda(b);
                auto x = la->var().value();
                auto M = la->var().type();
                auto N = la->expr();
                auto y = lb->var().value();
                auto K = lb->var().type();
                auto L = lb->expr();
                if (!is_convertible(M, K, delta)) return false;
                if (!is_free_var(L, x)) return is_convertible(
                    N,
                    substitute(L, y, x),
                    delta);
                auto z = get_fresh_var(x, y, N, L);
                return is_convertible(
                    substitute(N, x, z),
                    substitute(L, y, z),
                    delta);
            }
            case EpsilonType::AbstPi: {
                auto pa = pi(a);
                auto pb = pi(b);
                auto x = pa->var().value();
                auto M = pa->var().type();
                auto N = pa->expr();
                auto y = pb->var().value();
                auto K = pb->var().type();
                auto L = pb->expr();
                if (!is_convertible(M, K, delta)) return false;
                if (!is_free_var(L, x)) return is_convertible(
                    N,
                    substitute(L, y, x),
                    delta);
                auto z = get_fresh_var(x, y, N, L);
                return is_convertible(
                    substitute(N, x, z),
                    substitute(L, y, z),
                    delta);
            }
            case EpsilonType::Constant: {
                auto ca = constant(a);
                auto cb = constant(b);
                if (ca->name() == cb->name()) {
                    if (ca->args().size() != cb->args().size()) return is_convertible(delta_reduce(ca, delta), delta_reduce(cb, delta), delta);
                    for (size_t i = 0; i < ca->args().size(); ++i) {
                        if (!is_convertible(ca->args()[i], cb->args()[i], delta)) {
                            if (is_delta_reducible(ca, delta) && is_delta_reducible(cb, delta)) {
                                return is_convertible(delta_reduce(ca, delta), delta_reduce(cb, delta), delta);
                            }
                            return false;
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
                    "unknown etype (value = " << (int)(a->etype()) << ")",
                    __FILE__, __LINE__, __func__);
        }
    }
    // etype doesn't match
    if (b->etype() == EpsilonType::Constant) {
        auto cb = constant(b);
        if (is_delta_reducible(cb, delta)) return is_convertible(a, delta_reduce(cb, delta), delta);
        if (a->etype() != EpsilonType::Application) return false;
        std::shared_ptr<Term> ra = nullptr;
        if (ra = reduce_application(appl(a), delta)) return is_convertible(ra, cb, delta);
        return false;
    }
    switch (a->etype()) {
        case EpsilonType::Star:
        case EpsilonType::Square:
        case EpsilonType::Variable:
        case EpsilonType::AbstLambda:
        case EpsilonType::AbstPi: {
            switch (b->etype()) {
                case EpsilonType::Star:
                case EpsilonType::Square:
                case EpsilonType::Variable:
                case EpsilonType::AbstLambda:
                case EpsilonType::AbstPi:
                    return false;
                case EpsilonType::Application: {
                    std::shared_ptr<Term> rb = nullptr;
                    if (rb = reduce_application(appl(b), delta)) return is_convertible(a, rb, delta);
                    return false;
                }
                case EpsilonType::Constant:
                    check_true_or_exit(
                        false,
                        "contradiction; this line is logically unreachable. this must be a bug",
                        __FILE__, __LINE__, __func__);
                default:
                    check_true_or_exit(
                        false,
                        "unknown etype (value = " << (int)(b->etype()) << ")",
                        __FILE__, __LINE__, __func__);
            }
        }
        case EpsilonType::Application: {
            std::shared_ptr<Term> ra = nullptr;
            if (ra = reduce_application(appl(a), delta)) return is_convertible(ra, b, delta);
            return false;
        }
        case EpsilonType::Constant: {
            std::shared_ptr<Constant> ca = constant(a);
            if (is_delta_reducible(a, delta)) return is_convertible(delta_reduce(constant(a), delta), b, delta);
            if (b->etype() != EpsilonType::Application) return false;
            std::shared_ptr<Term> rb = nullptr;
            if (rb = reduce_application(appl(b), delta)) return is_convertible(a, rb, delta);
            return false;
        }
        default:
            check_true_or_exit(
                false,
                "unknown etype (value = " << (int)(a->etype()) << ")",
                __FILE__, __LINE__, __func__);
    }
    check_true_or_exit(
        false,
        "reached end of function, supposed to be unreachable",
        __FILE__, __LINE__, __func__);
}
