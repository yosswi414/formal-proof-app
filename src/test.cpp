#include <iostream>
#include <vector>
#include <memory>

#include "lambda.hpp"

int main(){
    std::shared_ptr<Term> x, y, z, u, v, e1, e2, e3;
    x = std::make_shared<Variable>('x');
    y = std::make_shared<Variable>('y');
    z = std::make_shared<Variable>('z');
    u = std::make_shared<Variable>('u');
    v = std::make_shared<Variable>('v');
    auto star = std::make_shared<Star>();
    auto sq = std::make_shared<Square>();
    auto appl = [](const std::shared_ptr<Term>& a, const std::shared_ptr<Term>& b) {
        return std::make_shared<Application>(a, b);
    };
    auto lambda = [](const std::shared_ptr<Term>& v, const std::shared_ptr<Term>& t, const std::shared_ptr<Term>& e) {
        return std::make_shared<AbstLambda>(std::dynamic_pointer_cast<Variable>(v), t, e);
    };
    auto pi = [](const std::shared_ptr<Term>& v, const std::shared_ptr<Term>& t, const std::shared_ptr<Term>& e) {
        return std::make_shared<AbstPi>(std::dynamic_pointer_cast<Variable>(v), t, e);
    };
    auto constant = [](const std::string& name, const std::vector<std::shared_ptr<Term>>& ts) {
        return std::make_shared<Constant>(name, ts);
    };

    unused(pi, constant);

#define bout(expr) \
    do { std::cerr << #expr " --> " << (expr ? "true" : "false") << std::endl; } while (false)
#define show(expr) \
    do { std::cerr << #expr " = " << expr << std::endl; } while (false)
#define subst_show(e, p, q) \
    do { std::cerr << "(" << e << ")[" << p << ":=" << q << "] --> " << substitute(e, p, q) << std::endl; } while (false);
#define sep() \
    do { std::cerr << "####################" << std::endl; } while (false)


    // p.11 Example 1.5.3

    auto l1 = appl(lambda(x, star, appl(x, lambda(z, star, appl(x, y)))), z);
    auto l2 = appl(lambda(u, star, appl(u, lambda(z, star, appl(u, y)))), z);
    auto l3 = appl(lambda(z, star, appl(z, lambda(x, star, appl(z, y)))), z);
    auto l4 = appl(lambda(y, star, appl(y, lambda(z, star, appl(y, y)))), z);
    auto l5 = appl(lambda(z, star, appl(z, lambda(z, star, appl(z, y)))), z);
    auto l6 = appl(lambda(u, star, appl(u, lambda(z, star, appl(u, y)))), v);

    show(l1);
    show(l2);
    show(l3);
    show(l4);
    show(l5);
    show(l6);

    bout(alpha_comp(l1, l1));
    bout(alpha_comp(l1, l2));
    bout(alpha_comp(l1, l3));
    bout(!alpha_comp(l1, l4));
    bout(!alpha_comp(l1, l5));
    bout(!alpha_comp(l1, l6));

    sep();

    auto la1 = lambda(x, star, lambda(y, star, appl(appl(x, z), y)));
    auto la2 = lambda(v, star, lambda(y, star, appl(appl(v, z), y)));
    auto la3 = lambda(v, star, lambda(u, star, appl(appl(v, z), u)));
    auto la4 = lambda(y, star, lambda(y, star, appl(appl(y, z), y)));
    auto la5 = lambda(z, star, lambda(y, star, appl(appl(z, z), y)));

    show(la1);
    show(la2);
    show(la3);
    show(la4);
    show(la5);

    bout(alpha_comp(la1, la2));
    bout(alpha_comp(la1, la3));
    bout(!alpha_comp(la1, la4));
    bout(!alpha_comp(la1, la5));

    sep();

    // p.12 Example 1.6.4

    auto lhs1 = lambda(y, star, appl(y, x));
    auto rhs1 = lambda(z, star, appl(z, appl(x, y)));
    show(lhs1);
    subst_show(lhs1, x, appl(x, y));
    show(rhs1);
    bout(alpha_comp(substitute(lhs1, x, appl(x, y)), rhs1));

    sep();

    // (2)
    auto lhs2 = lambda(x, star, appl(y, x));
    auto rhs2 = lambda(z, star, appl(y, z));
    show(lhs2);
    subst_show(lhs2, x, appl(x, y));
    show(rhs2);
    bout(alpha_comp(substitute(lhs2, x, appl(x, y)), rhs2));
    bout(alpha_comp(substitute(lhs2, x, appl(x, y)), lhs2));

    sep();

    // (3)
    auto lhs3 = lambda(x, star, lambda(y, star, appl(appl(z, z), x)));
    auto rhs3_1 = lambda(u, star, lambda(v, star, appl(appl(y, y), u)));
    auto rhs3_2 = lambda(x, star, lambda(v, star, appl(appl(y, y), x)));
    auto rhs3_3 = lambda(x, star, lambda(y, star, appl(appl(y, y), x)));
    show(lhs3);
    subst_show(lhs3, z, y);
    show(rhs3_1);
    show(rhs3_2);
    show(rhs3_3);
    bout(alpha_comp(substitute(lhs3, z, y), rhs3_1));
    bout(alpha_comp(substitute(lhs3, z, y), rhs3_2));
    bout(!alpha_comp(substitute(lhs3, z, y), rhs3_3));

    return 0;
}