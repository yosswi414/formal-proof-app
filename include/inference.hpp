#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "common.hpp"
#include "context.hpp"
#include "environment.hpp"
#include "lambda.hpp"

class TypeError {
  public:
    TypeError(const std::string& str, const std::shared_ptr<Term>& term, const std::shared_ptr<Context>& con);
    void puterror(std::ostream& os = std::cerr);

  private:
    std::string msg;
    std::shared_ptr<Term> term;
    std::shared_ptr<Context> con;
};

std::shared_ptr<Term> get_type(const std::shared_ptr<Term>& term, const std::shared_ptr<Environment>& delta, const std::shared_ptr<Context>& gamma);
