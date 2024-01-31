#pragma once

#include <memory>
#include <string>

#include "context.hpp"
#include "lambda.hpp"

class Definition {
  public:
    Definition(const std::shared_ptr<Context>& context,
               const std::string& cname,
               const std::shared_ptr<Term>& prop);

    Definition(const std::shared_ptr<Context>& context,
               const std::string& cname,
               const std::shared_ptr<Term>& proof,
               const std::shared_ptr<Term>& prop);

    Definition(const std::shared_ptr<Context>& context,
               const std::shared_ptr<Constant>& constant,
               const std::shared_ptr<Term>& prop);

    Definition(const std::shared_ptr<Context>& context,
               const std::shared_ptr<Constant>& constant,
               const std::shared_ptr<Term>& proof,
               const std::shared_ptr<Term>& prop);

    std::string string() const;
    std::string repr() const;
    std::string repr_new() const;
    std::string repr_book() const;

    bool is_prim() const;
    const std::shared_ptr<Context>& context() const;
    const std::string& definiendum() const;
    const std::shared_ptr<Term>& definiens() const;
    const std::shared_ptr<Term>& type() const;

    std::shared_ptr<Context>& context();
    std::string& definiendum();
    std::shared_ptr<Term>& definiens();
    std::shared_ptr<Term>& type();

  private:
    std::shared_ptr<Context> _context;
    std::string _definiendum;
    std::shared_ptr<Term> _definiens, _type;
};

bool equiv_def(const Definition& a, const Definition& b);
bool equiv_def(const std::shared_ptr<Definition>& a, const std::shared_ptr<Definition>& b);
