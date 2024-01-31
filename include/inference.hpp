#pragma once

#include <string>
#include <memory>
#include <iostream>
#include "common.hpp"
#include "lambda.hpp"
#include "context.hpp"
#include "environment.hpp"

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
