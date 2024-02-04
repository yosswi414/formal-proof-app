#include "context.hpp"

#include <memory>
#include <set>

#include "common.hpp"
#include "lambda.hpp"

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

Context Context::operator+(const Typed<Variable>& tv) const {
    return Context(*this) += tv;
}

Context& Context::operator+=(const Context& c) {
    for (auto&& tv : c) this->emplace_back(tv);
    return *this;
}

Context Context::operator+(const Context& c) const {
    return Context(*this) += c;
}

// std::set<char> free_var(const Context& con) {
//     std::set<char> FV;
//     for (auto&& tv : con) FV.insert(tv.value()->name());
//     return FV;
// }
std::set<std::string> free_var(const Context& con) {
    std::set<std::string> FV;
    for (auto&& tv : con) FV.insert(tv.value()->name());
    return FV;
}

bool is_free_var(const Context& con, const std::shared_ptr<Variable>& var) {
    auto fv = free_var(con);
    return fv.find(var->name()) != fv.end();
}

std::shared_ptr<Variable> get_fresh_var(const Context& con) {
    // std::set<char> univ;
    // for (char ch = 'A'; ch <= 'Z'; ++ch) univ.insert(ch);
    // for (char ch = 'a'; ch <= 'z'; ++ch) univ.insert(ch);
    // set_minus_inplace(univ, free_var(con));
    // if (!univ.empty()) return variable(*univ.begin());
    // check_true_or_exit(false, "out of fresh variable",
    //                    __FILE__, __LINE__, __func__);
    auto univ = char_vars_set();
    set_minus_inplace(univ, free_var(con));
    if (!univ.empty()) {
        for (auto&& ch : _preferred_names) {
            auto itr = univ.find({ch});
            if (itr != univ.end()) return variable({ch});
        }
        return variable(*univ.begin());
    }
    return get_fresh_var_depleted();
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
    if (a == b) return true;
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
    if (a == b) return true;
    return equiv_context(*a, *b);
}

bool has_variable(const std::shared_ptr<Context>& g, const std::shared_ptr<Variable>& v) {
    for (auto&& tv : *g) {
        if (alpha_comp(tv.value(), v)) return true;
    }
    return false;
}

bool has_variable(const std::shared_ptr<Context>& g, const std::shared_ptr<Term>& v) {
    return has_variable(g, variable(v));
}

bool has_variable(const std::shared_ptr<Context>& g, char v) {
    return has_variable(g, std::make_shared<Variable>(v));
}
