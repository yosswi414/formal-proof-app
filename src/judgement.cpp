#include <string>
#include <memory>

#include "lambda.hpp"
#include "context.hpp"
#include "environment.hpp"
#include "common.hpp"
#include "judgement.hpp"

Judgement::Judgement(const std::shared_ptr<Environment>& env,
                     const std::shared_ptr<Context>& context,
                     const std::shared_ptr<Term>& proof,
                     const std::shared_ptr<Term>& prop)
    : _env(env), _context(context), _term(proof), _type(prop) {}
std::string Judgement::string(bool inSingleLine, size_t indentSize) const {
    std::string res("");
    std::string indent_ex_1(indentSize, '\t');
    std::string indent_ex(inSingleLine ? 0 : indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    res += indent_ex_1 + "Judge<<" + eol;
    res += _env->string(inSingleLine, inSingleLine ? 0 : indentSize + 1);
    res += " ;" + eol + indent_ex + indent_in + _context->string();
    res += " " + TURNSTILE + " " + _term->string();
    res += " : " + _type->string();
    res += eol + indent_ex + ">>";
    return res;
}

std::string Judgement::string_brief(bool inSingleLine, size_t indentSize) const {
    std::string res("");
    std::string indent_ex_1(indentSize, '\t');
    std::string indent_ex(inSingleLine ? 0 : indentSize, '\t'), indent_in(inSingleLine ? "" : "\t"), eol(inSingleLine ? " " : "\n");
    res += indent_ex_1 + "Judge<<" + eol;
    res += _env->string_brief(inSingleLine, inSingleLine ? 0 : indentSize + 1);
    res += " ;" + eol + indent_ex + indent_in + _context->string();
    res += " " + TURNSTILE + " " + _term->string();
    res += " : " + _type->string();
    res += eol + indent_ex + ">>";
    return res;
}

const std::shared_ptr<Environment>& Judgement::env() const { return _env; }
const std::shared_ptr<Context>& Judgement::context() const { return _context; }
const std::shared_ptr<Term>& Judgement::term() const { return _term; }
const std::shared_ptr<Term>& Judgement::type() const { return _type; }

std::shared_ptr<Environment>& Judgement::env() { return _env; }
std::shared_ptr<Context>& Judgement::context() { return _context; }
std::shared_ptr<Term>& Judgement::term() { return _term; }
std::shared_ptr<Term>& Judgement::type() { return _type; }
