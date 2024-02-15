#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "definition.hpp"
#include "lambda.hpp"

class Environment : public std::vector<std::shared_ptr<Definition>> {
  public:
    Environment();
    Environment(const std::vector<std::shared_ptr<Definition>>& defs);
    Environment(const std::string& fname);
    std::string string(bool inSingleLine = true, size_t indentSize = 0) const;
    std::string string_brief(bool inSingleLine, size_t indentSize) const;
    std::string repr() const;
    std::string repr_new() const;

    int lookup_index(const std::string& cname) const;
    int lookup_index(const std::shared_ptr<Constant>& c) const;

    const std::shared_ptr<Definition> lookup_def(const std::string& cname) const;
    const std::shared_ptr<Definition> lookup_def(const std::shared_ptr<Constant>& c) const;

    Environment& operator+=(const std::shared_ptr<Definition>& def);
    Environment operator+(const std::shared_ptr<Definition>& def) const;

  private:
    mutable std::map<std::string, size_t> _def_index;
};

bool equiv_env(const Environment& a, const Environment& b);
bool equiv_env(const std::shared_ptr<Environment>& a, const std::shared_ptr<Environment>& b);

bool has_constant(const std::shared_ptr<Environment>& env, const std::string& name);
bool has_definition(const std::shared_ptr<Environment>& env, const Definition& def);

bool is_delta_reducible(const std::shared_ptr<Term>& term, const Environment& delta);

int expr_rank(const std::shared_ptr<Term>& term, const Environment& delta);
std::shared_ptr<Term> delta_reduce(const std::shared_ptr<Constant>& term, const Environment& delta);
std::shared_ptr<Term> delta_nf_above(const std::shared_ptr<Term>& term, const Environment& delta, int idx);
std::shared_ptr<Term> delta_nf(const std::shared_ptr<Term>& term, const Environment& delta);
std::shared_ptr<Term> NF_above(const std::shared_ptr<Term>& term, const Environment& delta, int idx);
std::shared_ptr<Term> NF(const std::shared_ptr<Term>& term, const Environment& delta);
std::shared_ptr<Term> NF(const std::shared_ptr<Term>& term, const std::shared_ptr<Environment>& delta);

bool is_constant_defined(const std::string& cname, const Environment& delta);
bool is_constant_primitive(const std::string& cname, const Environment& delta);

bool is_convertible(const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b, const Environment& delta);

std::set<std::string> extract_constant(const Environment& env);
std::set<std::string> extract_constant(const std::shared_ptr<Environment>& env);
