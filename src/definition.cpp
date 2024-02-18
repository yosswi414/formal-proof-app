#include "definition.hpp"

#include <memory>
#include <string>

#include "context.hpp"
#include "lambda.hpp"

Definition::Definition(const std::shared_ptr<Context>& context,
                       const std::string& cname,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(cname),
      _definiens(nullptr),
      _type(prop) {}

Definition::Definition(const std::shared_ptr<Context>& context,
                       const std::string& cname,
                       const std::shared_ptr<Term>& proof,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(cname),
      _definiens(proof),
      _type(prop) {}

Definition::Definition(const std::shared_ptr<Context>& context,
                       const std::shared_ptr<Constant>& constant,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(constant->name()),
      _definiens(nullptr),
      _type(prop) {}

Definition::Definition(const std::shared_ptr<Context>& context,
                       const std::shared_ptr<Constant>& constant,
                       const std::shared_ptr<Term>& proof,
                       const std::shared_ptr<Term>& prop)
    : _context(context),
      _definiendum(constant->name()),
      _definiens(proof),
      _type(prop) {}

std::string Definition::string() const {
    std::string res;
    res = (_definiens ? "Def< " : "Def-prim< ");
    res += _context->string();
    res += " " + DEFINITION_SEPARATOR + " " + _definiendum;
    res += " := " + (_definiens ? _definiens->string() : DOUBLE_BOTTOM);
    res += " : " + _type->string();
    res += " >";
    return res;
}

std::string Definition::repr() const {
    std::string res;
    res = "def2\n";
    Context con_new = *_context;
    auto term = _definiens ? copy(_definiens) : nullptr;
    auto type = copy(_type);
    for (size_t i = 0; i < con_new.size(); ++i) {
        auto& tv = con_new[i];
        if (tv.value()->name().size() == 1) continue;
        auto var_old = tv.value();
        auto var_new = get_fresh_var(con_new);
        tv.value() = var_new;
        for (size_t j = i + 1; j < con_new.size(); ++j) {
            con_new[j].type() = substitute(con_new[j].type(), var_old, var_new);
        }
        tv.type() = rename_var_short(tv.type());
        if (term) term = substitute(term, var_old, var_new);
        type = substitute(type, var_old, var_new);
    }
    term = rename_var_short(term);
    type = rename_var_short(type);

    res += con_new.repr();
    res += _definiendum + "\n";
    res += (term ? term->repr() : "#") + "\n";
    res += type->repr() + "\n";
    res += "edef2\n";
    return res;
}

std::string Definition::repr_new() const {
    std::string res;
    res = "def2\n";
    res += _context->repr_new();
    res += _definiendum + " := " + (_definiens ? _definiens->repr_new() : "#") + " : " + _type->repr_new() + "\n";
    res += "edef2\n";
    return res;
}

std::string Definition::repr_book() const {
    std::stringstream ss;
    ss << _context->repr_book() << " |> ";
    ss << _definiendum << " := ";
    ss << (_definiens ? _definiens->repr_book() : "#") << " : ";
    ss << _type->repr_book();
    return ss.str();
}

bool Definition::is_prim() const { return !_definiens; }
const std::shared_ptr<Context>& Definition::context() const { return _context; }
const std::string& Definition::definiendum() const { return _definiendum; }
const std::shared_ptr<Term>& Definition::definiens() const { return _definiens; }
const std::shared_ptr<Term>& Definition::type() const { return _type; }

std::shared_ptr<Context>& Definition::context() { return _context; }
std::string& Definition::definiendum() { return _definiendum; }
std::shared_ptr<Term>& Definition::definiens() { return _definiens; }
std::shared_ptr<Term>& Definition::type() { return _type; }

bool equiv_def(const Definition& a, const Definition& b) {
    check_true_or_ret_false(
        equiv_context(a.context(), b.context()),
        "equiv_def(): context doesn't match",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        a.definiendum() == b.definiendum(),
        "equiv_def(): definiendum doesn't match",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        a.is_prim() == b.is_prim(),
        "equiv_def(): one is primitive definition and another is not",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        a.is_prim() || alpha_comp(a.definiens(), b.definiens()),
        "equiv_def(): definiens doesn't match",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false(
        alpha_comp(a.type(), b.type()),
        "equiv_def(): type doesn't match",
        __FILE__, __LINE__, __func__);
    return true;
}

bool equiv_def(const std::shared_ptr<Definition>& a, const std::shared_ptr<Definition>& b) {
    if (a == b) return true;
    return equiv_def(*a, *b);
}

std::set<std::string> extract_constant(const Definition& def) {
    std::set<std::string> constants;
    constants = extract_constant(def.context());
    if (!def.is_prim()) set_union_inplace(constants, extract_constant(def.definiens()));
    set_union_inplace(constants, extract_constant(def.type()));
    return constants;
}

std::set<std::string> extract_constant(const std::shared_ptr<Definition>& def) {
    return extract_constant(*def);
}
