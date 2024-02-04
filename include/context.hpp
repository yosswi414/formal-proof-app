#pragma once

#include <vector>
#include <string>
#include <memory>
#include <set>

#include "lambda.hpp"

class Context : public std::vector<Typed<Variable>> {
  public:
    Context();
    Context(const std::vector<Typed<Variable>>& tvars);
    std::string string() const;
    std::string repr() const;
    std::string repr_new() const;
    std::string repr_book() const;

    Context& operator+=(const Typed<Variable>& tv);
    Context operator+(const Typed<Variable>& tv) const;
    Context& operator+=(const Context& c);
    Context operator+(const Context& c) const;
};

bool equiv_context_n(const Context& a, const Context& b, size_t n);
bool equiv_context_n(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b, size_t n);
bool equiv_context(const Context& a, const Context& b);
bool equiv_context(const std::shared_ptr<Context>& a, const std::shared_ptr<Context>& b);

bool has_variable(const std::shared_ptr<Context>& g, const std::shared_ptr<Variable>& v);
bool has_variable(const std::shared_ptr<Context>& g, const std::shared_ptr<Term>& v);
bool has_variable(const std::shared_ptr<Context>& g, char v);

// std::set<char> free_var(const Context& con);
std::set<std::string> free_var(const Context& con);
// template <class... Ts>
// std::set<char> free_var(const Context& con, Ts... data) {
//     return set_union(free_var(con), free_var(data...));
// }
template <class... Ts>
std::set<std::string> free_var(const Context& con, Ts... data) {
    return set_union(free_var(con), free_var(data...));
}

bool is_free_var(const Context& con, const std::shared_ptr<Variable>& var);

std::shared_ptr<Variable> get_fresh_var(const Context& con);
