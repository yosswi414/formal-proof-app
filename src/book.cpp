#include <vector>
#include <string>
#include <sstream>

#include "judgement.hpp"
#include "book.hpp"

Book::Book(bool skip_check) : std::vector<Judgement>{}, _skip_check{skip_check} {}
Book::Book(const std::vector<Judgement>& list) : std::vector<Judgement>(list) {}
Book::Book(const std::string& scriptname, size_t limit) : Book(false) {
    read_script(scriptname, limit);
}
Book::Book(const FileData& fdata, size_t limit) : Book(false) {
    read_script(fdata, limit);
}

void Book::read_script(const FileData& fdata, size_t limit) {
    std::stringstream ss;
    auto errmsg = [](const std::string& op, size_t lno) {
        return op + ": wrong format (line " + std::to_string(lno + 1) + ")";
    };
    for (size_t i = 0; i < limit; ++i) {
        ss << fdata[i];
        int lno;
        std::string op;
        ss >> lno;
        if (lno == -1) break;
        ss >> op;
        if (op == "sort") {
            sort();
        } else if (op == "var") {
            size_t idx;
            char x;
            check_true_or_exit(
                ss >> idx >> x,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            var(idx, x);
        } else if (op == "weak") {
            size_t idx1, idx2;
            char x;
            check_true_or_exit(
                ss >> idx1 >> idx2 >> x,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            weak(idx1, idx2, x);
        } else if (op == "form") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            form(idx1, idx2);
        } else if (op == "appl") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            appl(idx1, idx2);
        } else if (op == "abst") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            abst(idx1, idx2);
        } else if (op == "conv") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            conv(idx1, idx2);
        } else if (op == "def") {
            size_t idx1, idx2;
            std::string a;
            check_true_or_exit(
                ss >> idx1 >> idx2 >> a,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            def(idx1, idx2, a);
        } else if (op == "defpr") {
            size_t idx1, idx2;
            std::string a;
            check_true_or_exit(
                ss >> idx1 >> idx2 >> a,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            defpr(idx1, idx2, a);
        } else if (op == "inst") {
            size_t idx0, n, p;
            check_true_or_exit(
                ss >> idx0 >> n,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            std::vector<size_t> idxs(n);
            for (auto&& ki : idxs) {
                check_true_or_exit(
                    ss >> ki,
                    errmsg(op, i),
                    __FILE__, __LINE__, __func__);
            }
            check_true_or_exit(
                ss >> p,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);
            inst(idx0, n, idxs, p);
        } else if (op == "cp") {
            size_t idx;
            check_true_or_exit(
                ss >> idx,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            cp(idx);
        } else if (op == "sp") {
            size_t idx, n;
            check_true_or_exit(
                ss >> idx >> n,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            sp(idx, n);
        } else if (op == "tp") {
            size_t idx;
            check_true_or_exit(
                ss >> idx,
                errmsg(op, i),
                __FILE__, __LINE__, __func__);

            tp(idx);
        } else {
            check_true_or_exit(
                false,
                "not implemented (token: " << op << ")",
                __FILE__, __LINE__, __func__);
        }
        ss.clear();
        ss.str("");
    }
}

void Book::read_script(const std::string& scriptname, size_t limit) {
    read_script(FileData(scriptname), limit);
}

// inference rules
void Book::sort() {
    this->emplace_back(
        std::make_shared<Environment>(),
        std::make_shared<Context>(),
        star,
        sq);
}
void Book::var(size_t m, char x) {
    if (!_skip_check && !is_var_applicable(*this, m, x)) {
        throw InferenceError()
            << "var at line "
            << this->size() << " not applicable "
            << "(idx = " << m << ", var = " << x << ")";
    }

    const auto& judge = (*this)[m];
    auto vx = variable(x);
    auto A = judge.term();
    this->emplace_back(
        judge.env(),
        std::make_shared<Context>(*judge.context() + Typed<Variable>(vx, A)),
        vx, A);
}
void Book::weak(size_t m, size_t n, char x) {
    if (!_skip_check && !is_weak_applicable(*this, m, n, x)) {
        throw InferenceError()
            << "weak at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ", var = " << x << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto vx = variable(x);
    auto A = judge1.term();
    auto B = judge1.type();
    auto C = judge2.term();
    this->emplace_back(
        judge1.env(),
        std::make_shared<Context>(*judge1.context() + Typed<Variable>(vx, C)),
        A, B);
}
void Book::form(size_t m, size_t n) {
    if (!_skip_check && !is_form_applicable(*this, m, n)) {
        throw InferenceError()
            << "form at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto x = judge2.context()->back().value();
    auto A = judge1.term();
    auto B = judge2.term();
    auto s2 = judge2.type();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        pi(x, A, B), s2);
}

void Book::appl(size_t m, size_t n) {
    if (!_skip_check && !is_appl_applicable(*this, m, n)) {
        throw InferenceError()
            << "appl at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto M = judge1.term();
    auto N = judge2.term();
    auto B = pi(judge1.type())->expr();
    auto x = pi(judge1.type())->var().value();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        ::appl(M, N),
        substitute(B, x, N));
}

void Book::abst(size_t m, size_t n) {
    if (!_skip_check && !is_abst_applicable(*this, m, n)) {
        throw InferenceError()
            << "abst at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto M = judge1.term();
    auto x = judge1.context()->back().value();
    auto A = judge1.context()->back().type();
    auto B = judge1.type();
    this->emplace_back(
        judge2.env(),
        judge2.context(),
        lambda(x, A, M),
        pi(x, A, B));
}

void Book::conv(size_t m, size_t n) {
    if (!_skip_check && !is_conv_applicable(*this, m, n)) {
        throw InferenceError()
            << "conv at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto A = judge1.term();
    auto B2 = judge2.term();
    this->emplace_back(
        judge1.env(),
        judge1.context(),
        A, B2);
}

void Book::def(size_t m, size_t n, const std::string& a) {
    if (!_skip_check && !is_def_applicable(*this, m, n, a)) {
        throw InferenceError()
            << "def at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ", name = " << a << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto K = judge1.term();
    auto L = judge1.type();
    auto M = judge2.term();
    auto N = judge2.type();
    const auto& xAs = judge2.context();
    std::vector<std::shared_ptr<Term>> xs;
    for (auto&& xA : *xAs) xs.push_back(xA.value());
    this->emplace_back(
        std::make_shared<Environment>(
            *judge1.env() +
            std::make_shared<Definition>(
                xAs, constant(a, xs), M, N)),
        judge1.context(),
        K, L);
}

void Book::defpr(size_t m, size_t n, const std::string& a) {
    if (!_skip_check && !is_def_applicable(*this, m, n, a)) {
        throw InferenceError()
            << "defpr at line "
            << this->size() << " not applicable "
            << "(idx1 = " << m << ", idx2 = " << n << ", name = " << a << ")";
    }
    const auto& judge1 = (*this)[m];
    const auto& judge2 = (*this)[n];
    auto K = judge1.term();
    auto L = judge1.type();
    auto N = judge2.term();
    auto& xAs = judge2.context();
    std::vector<std::shared_ptr<Term>> xs;
    for (auto&& xA : *xAs) xs.push_back(xA.value());
    this->emplace_back(
        std::make_shared<Environment>(
            *judge1.env() +
            std::make_shared<Definition>(xAs, constant(a, xs), N)),
        judge1.context(),
        K, L);
}

void Book::inst(size_t m, size_t n, const std::vector<size_t>& k, size_t p) {
    if (!_skip_check && !is_inst_applicable(*this, m, n, k, p)) {
        throw InferenceError()
            << "inst at line "
            << this->size() << " not applicable "
            << "(idx = " << m << ", n = " << n << ", k = " << to_string(k) << ", p = " << p << ")";
    }
    const auto& judge = (*this)[m];
    auto& D = (*judge.env())[p];

    auto N = D->type();

    std::vector<std::shared_ptr<Variable>> xs;
    std::vector<std::shared_ptr<Term>> Us;
    for (size_t i = 0; i < n; ++i) {
        xs.push_back((*D->context())[i].value());
        Us.push_back((*this)[k[i]].term());
    }

    this->emplace_back(
        judge.env(),
        judge.context(),
        constant(D->definiendum(), Us),
        substitute(N, xs, Us));
}

void Book::cp(size_t m) {
    this->emplace_back((*this)[m]);
}

void Book::sp(size_t m, size_t n) {
    const auto& judge = (*this)[m];
    auto& tv = (*judge.context())[n];
    this->emplace_back(
        judge.env(),
        judge.context(),
        tv.value(),
        tv.type());
}

void Book::tp(size_t m) {
    if (!_skip_check && !is_tp_applicable(*this, m)) {
        throw InferenceError()
            << "tp at line "
            << this->size() << " not applicable "
            << "(idx = " << m << ")";
    }
    const auto& judge = (*this)[m];
    this->emplace_back(
        judge.env(),
        judge.context(),
        sq,
        sq);
}

std::string Book::string() const {
    std::string res("Book[[");
    bool singleLine = true;
    int indentSize = 1;
    if (this->size() > 0) res += "\n[0]" + (*this)[0].string_brief(singleLine, indentSize);
    for (size_t i = 1; i < this->size(); ++i) res += ",\n[" + std::to_string(i) + "]" + (*this)[i].string_brief(singleLine, indentSize);
    res += "\n]]";
    return res;
}
std::string Book::repr() const {
    std::stringstream ss;
    // defs enum
    for (size_t dno = 0; dno < this->env().size(); ++dno) {
        ss << "D" << dno << " : " << this->env()[dno]->repr_book() << std::endl;
    }
    if (this->env().size() > 0) ss << "--------------" << std::endl;
    for (size_t lno = 0; lno < this->size(); ++lno) {
        const auto& judge = (*this)[lno];
        ss << lno << " : ";
        if (this->env().size() > 0) {
            if (judge.env()->size() > 0) ss << "D" << def_num((*judge.env())[0]);
            for (size_t dno = 1; dno < judge.env()->size(); ++dno) {
                ss << ",D" << def_num((*judge.env())[dno]);
            }
        } else if (judge.env()->size() > 0) {
            ss << "env(#defs = " << judge.env()->size() << ")";
        }
        ss << " ; ";
        // output context
        ss << judge.context()->repr_book() << " |- ";
        ss << judge.term()->repr_book() << " : ";
        ss << judge.type()->repr_book() << "\n";
    }
    return ss.str();
}
std::string Book::repr_new() const {
    std::stringstream ss;
    // defs enum
    for (size_t dno = 0; dno < this->env().size(); ++dno) {
        ss << "D" << dno << " : " << this->env()[dno]->repr_book() << std::endl;
    }
    if (this->env().size() > 0) ss << "--------------" << std::endl;
    for (size_t lno = 0; lno < this->size(); ++lno) {
        const auto& judge = (*this)[lno];
        ss << lno << " : ";
        if (this->env().size() > 0) {
            if (judge.env()->size() > 0) ss << "D" << def_num((*judge.env())[0]);
            for (size_t dno = 1; dno < judge.env()->size(); ++dno) {
                ss << ",D" << def_num((*judge.env())[dno]);
            }
        } else if (judge.env()->size() > 0) {
            ss << "env(#defs = " << judge.env()->size() << ")";
        }
        ss << " ; ";
        // output context
        ss << judge.context()->repr_book() << " |- ";
        ss << judge.term()->repr_new() << " : ";
        ss << judge.type()->repr_new() << "\n";
    }
    return ss.str();
}

void Book::read_def_file(const std::string& fname) {
    this->_env = Environment(fname);
    for (size_t dno = 0; dno < this->env().size(); ++dno) {
        this->_def_dict[this->env()[dno]->definiendum()] = dno;
    }
}

const Environment& Book::env() const { return _env; }
int Book::def_num(const std::shared_ptr<Definition>& def) const {
    return _env.lookup_index(def->definiendum());
}

bool is_var_applicable(const Book& book, size_t idx, char var) {
    const auto& judge = book[idx];
    check_true_or_ret_false_err(
        is_sort(judge.type()),
        "type of judgement is neither * nor @"
            << std::endl
            << "type: " << judge.type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        !has_variable(judge.context(), var),
        "context already has a variable " << var,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_weak_applicable(const Book& book, size_t idx1, size_t idx2, char var) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        equiv_context(judge1.context(), judge2.context()),
        "context doesn't match"
            << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        is_sort(judge2.type()),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        !has_variable(judge1.context(), var),
        "context already has a variable " << var,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_form_applicable(const Book& book, size_t idx1, size_t idx2) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        equiv_context_n(judge1.context(), judge2.context(), judge1.context()->size()),
        "first " << judge1.context()->size() << " statements of context doesn't match" << std::endl
                 << "context 1: " << judge1.context() << std::endl
                 << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        judge1.context()->size() + 1 == judge2.context()->size(),
        "size of context is not appropriate"
            << std::endl
            << "size 1: " << judge1.context()->size() << std::endl
            << "size 2: " << judge2.context()->size() << " (should have 1 more)",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        alpha_comp(judge1.term(), judge2.context()->back().type()),
        "term of 1st judge and type of last statement of context doesn't match"
            << std::endl
            << "term: " << judge1.term() << std::endl
            << "type: " << judge2.context()->back().type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        is_sort(judge1.type()),
        "type of 1st judgement is neither * nor @"
            << std::endl
            << "type: " << judge1.type(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        is_sort(judge2.type()),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_appl_applicable(const Book& book, size_t idx1, size_t idx2) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        equiv_context(judge1.context(), judge2.context()),
        "context doesn't match"
            << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        judge1.type()->etype() == EpsilonType::AbstPi,
        "type of 1st judgement is not a pi abstraction"
            << std::endl
            << "type: " << judge1.type(),
        __FILE__, __LINE__, __func__);
    auto p = pi(judge1.type());
    check_true_or_ret_false_err(
        alpha_comp(p->var().type(), judge2.type()),
        "type of bound variable is not alpha-equivalent to the type of 2nd judgement"
            << std::endl
            << "type pi: " << p->var().type() << std::endl
            << "type  2: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_abst_applicable(const Book& book, size_t idx1, size_t idx2) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "abst: "
            << "environment doesn't match" << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        equiv_context_n(judge1.context(), judge2.context(), judge2.context()->size()),
        "abst: "
            << "first " << judge2.context()->size() << " statements of context doesn't match" << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        judge1.context()->size() == judge2.context()->size() + 1,
        "abst: "
            << "size of context is not appropriate" << std::endl
            << "size 1: " << judge1.context()->size() << std::endl
            << "size 2: " << judge2.context()->size() << " (should have 1 less)",
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        judge2.term()->etype() == EpsilonType::AbstPi,
        "abst: "
            << "term of 2nd judgement is not a pi abstraction" << std::endl
            << "term: " << judge2.term(),
        __FILE__, __LINE__, __func__);
    auto p = pi(judge2.term());
    auto x = judge1.context()->back().value();
    auto A = judge1.context()->back().type();
    auto B = judge1.type();
    check_true_or_ret_false_err(
        alpha_comp(p->var().value(), x),
        "abst: "
            << "bound variable is not alpha-equivalent to the last variable in context" << std::endl
            << "bound: " << p->var().value() << std::endl
            << "    x: " << x,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        alpha_comp(p->var().type(), A),
        "abst: "
            << "type of bound variable is not alpha-equivalent to the last type in context" << std::endl
            << "bound type: " << p->var().type() << std::endl
            << " last type: " << A,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        alpha_comp(p->expr(), B),
        "abst: "
            << "expr of pi abstraction is not alpha-equivalent to the type of 1st judgement" << std::endl
            << "expr pi: " << p->expr() << std::endl
            << " type 1: " << B,
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        is_sort(judge2.type()),
        "abst: "
            << "type of 2nd judgement is not a sort" << std::endl
            << "type: " << judge2.type() << std::endl,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_conv_applicable(const Book& book, size_t idx1, size_t idx2) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        equiv_context(judge1.context(), judge2.context()),
        "context doesn't match"
            << std::endl
            << "context 1: " << judge1.context() << std::endl
            << "context 2: " << judge2.context(),
        __FILE__, __LINE__, __func__);
    auto B1 = judge1.type();
    auto B2 = judge2.term();
    auto s = judge2.type();
    check_true_or_ret_false_err(
        is_convertible(B1, B2, *judge1.env()),
        "type of 1st judgement and term of 2nd judgement are not beta-delta-equivalent"
            << std::endl
            << "type 1: " << B1->repr_new() << std::endl
            << "term 2: " << B2->repr_new(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        is_sort(s),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << s,
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_def_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        !has_constant(judge1.env(), name),
        "environment of 1st judgement already has the definition of \""
            << name << "\"" << std::endl
            << "env: " << judge1.env(),
        __FILE__, __LINE__, __func__);
    ;
    return true;
}

bool is_def_prim_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name) {
    const auto& judge1 = book[idx1];
    const auto& judge2 = book[idx2];
    check_true_or_ret_false_err(
        equiv_env(judge1.env(), judge2.env()),
        "environment doesn't match"
            << std::endl
            << "env 1: " << judge1.env() << std::endl
            << "env 2: " << judge2.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        !has_constant(judge1.env(), name),
        "environment of 1st judgement already has the definition of \""
            << name << "\"" << std::endl
            << "env: " << judge1.env(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        is_sort(judge2.type()),
        "type of 2nd judgement is neither * nor @"
            << std::endl
            << "type: " << judge2.type(),
        __FILE__, __LINE__, __func__);
    return true;
}

bool is_inst_applicable(const Book& book, size_t idx, size_t n, const std::vector<size_t>& k, size_t p) {
    const auto& judge = book[idx];

    check_true_or_ret_false_err(
        k.size() == n,
        "length of k and n doesn't match (this seems to be a bug. please report with your input)"
            << std::endl
            << "length of k: " << k.size() << " (should be n = " << n << ")",
        __FILE__, __LINE__, __func__);
    for (size_t i = 0; i < n; ++i) {
        check_true_or_ret_false_err(
            equiv_env(judge.env(), book[k[i]].env()),
            "environment doesn't match"
                << std::endl
                << "env [" << idx << "]: " << judge.env() << std::endl
                << "env [" << k[i] << "]: " << book[k[i]].env(),
            __FILE__, __LINE__, __func__);
        check_true_or_ret_false_err(
            equiv_context(judge.context(), book[k[i]].context()),
            "context doesn't match"
                << std::endl
                << "context [" << idx << "]: " << judge.context() << std::endl
                << "context [" << k[i] << "]: " << book[k[i]].context(),
            __FILE__, __LINE__, __func__);
    }

    // check_true_or_ret_false_err(judge.context()->size() == n,

    const std::shared_ptr<Definition>& D = (*judge.env())[p];

    std::vector<std::shared_ptr<Term>> Us;
    std::vector<std::shared_ptr<Variable>> xs;
    for (size_t i = 0; i < n; ++i) {
        auto A = (*D->context())[i].type();
        auto V = book[k[i]].type();
        // check V == A[xs := Us]
        auto AxU = substitute(A, xs, Us);
        check_true_or_ret_false_err(
            alpha_comp(V, AxU),
            "type equivalence (U_i : A_i[x_1:=U_1,..] for all i) doesn't hold"
                << std::endl
                << "k_i: " << k[i] << std::endl
                << "A[x:=U] (V should be): " << AxU << std::endl
                << "      V (actual type): " << V << std::endl
                << "                    A: " << A << std::endl
                << "                    i: " << i << std::endl
                << "                    xs: " << to_string(xs) << std::endl
                << "                    Us: " << to_string(Us) << std::endl
                << "  the def failed inst: " << D << std::endl,
            __FILE__, __LINE__, __func__);
        xs.push_back((*D->context())[i].value());
        Us.push_back(book[k[i]].term());
    }

    check_true_or_ret_false_err(
        judge.term()->etype() == EpsilonType::Star,
        "term of 1st judgement is not *"
            << std::endl
            << "term: " << judge.term(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        judge.type()->etype() == EpsilonType::Square,
        "type of 1st judgement is not @"
            << std::endl
            << "type: " << judge.type(),
        __FILE__, __LINE__, __func__);

    return true;
}

bool is_tp_applicable(const Book& book, size_t idx) {
    const auto& judge = book[idx];
    check_true_or_ret_false_err(
        judge.term()->etype() == EpsilonType::Star,
        "term of judgement is not *"
            << std::endl
            << "term: " << judge.term(),
        __FILE__, __LINE__, __func__);
    check_true_or_ret_false_err(
        judge.type()->etype() == EpsilonType::Square,
        "type of judgement is not @"
            << std::endl
            << "type: " << judge.type(),
        __FILE__, __LINE__, __func__);
    return true;
}
