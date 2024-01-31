#pragma once

#include <memory>
#include <string>

#include "environment.hpp"
#include "lambda.hpp"

class Judgement {
  public:
    Judgement(const std::shared_ptr<Environment>& env,
              const std::shared_ptr<Context>& context,
              const std::shared_ptr<Term>& proof,
              const std::shared_ptr<Term>& prop);
    std::string string(bool inSingleLine = true, size_t indentSize = 0) const;
    std::string string_brief(bool inSingleLine, size_t indentSize) const;

    const std::shared_ptr<Environment>& env() const;
    const std::shared_ptr<Context>& context() const;
    const std::shared_ptr<Term>& term() const;
    const std::shared_ptr<Term>& type() const;

    std::shared_ptr<Environment>& env();
    std::shared_ptr<Context>& context();
    std::shared_ptr<Term>& term();
    std::shared_ptr<Term>& type();

  private:
    std::shared_ptr<Environment> _env;
    std::shared_ptr<Context> _context;
    std::shared_ptr<Term> _term, _type;
};
