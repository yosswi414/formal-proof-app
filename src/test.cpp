#include <iostream>
#include <memory>
#include <vector>

#include "lambda.hpp"
#include "parser.hpp"

#define bout(expr) \
    do { std::cerr << #expr " --> " << (expr ? "true" : "false") << std::endl; } while (false)
#define show(expr) \
    do { std::cerr << #expr " = " << expr << std::endl; } while (false)
#define subst_show(e, p, q) \
    do { std::cerr << "(" << e << ")[" << p << ":=" << q << "] --> " << substitute(e, p, q) << std::endl; } while (false);
#define sep() \
    do { std::cerr << "####################" << std::endl; } while (false)

void test_alpha_subst() {
    std::shared_ptr<Term> e1, e2, e3;
    auto x = variable('x');
    auto y = variable('y');
    auto z = variable('z');
    auto u = variable('u');
    auto v = variable('v');

    std::cerr << "[textbook p.11 Example 1.5.3]" << std::endl;

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

    std::cerr << "[textbook p.12 Example 1.6.4]" << std::endl;

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

    sep();
}

void test_subst() {
    std::cerr << "[substitution test]" << std::endl;
    auto A = variable('A');
    auto a = variable('a');
    auto b = variable('b');
    auto c = variable('c');
    auto d = variable('d');
    auto e = variable('e');
    auto f = variable('f');

    show(A);
    show(a);
    show(b);
    show(c);
    show(d);

    auto pre = pi(b, pi(a, A, c), pi(e, pi(d, pi(a, A, pi(d, star, d)), c), c));
    show(pre);
    auto N = A;

    auto sub = substitute(pre, c, N);
    show(sub);

    auto ans = pi(f, pi(b, A, A), pi(a, pi(a, pi(c, A, pi(b, star, b)), A), A));
    show(ans);
    bout(alpha_comp(sub, ans));

    sep();
}

void test_reduction1(const Environment& delta) {
    std::cerr << "[beta / delta reduction test]" << std::endl;
    auto S = variable('S');
    auto P = variable('P');
    auto v = variable('v');
    auto a = variable('a');
    auto b = variable('b');
    auto c = variable('c');
    auto d = variable('d');
    auto e = variable('e');
    auto f = variable('f');
    auto g = variable('g');
    auto h = variable('h');
    auto i = variable('i');
    auto j = variable('j');
    auto x = variable('x');
    auto y = variable('y');
    auto A = variable('A');
    auto B = variable('B');

    std::shared_ptr<Term> exprB = constant("forall", {S, lambda(a, S, constant("not", {constant("not", {appl(P, a)})}))});
    std::shared_ptr<Term> ansnB = pi(c, S, pi(b, pi(b, appl(P, c), pi(a, star, a)), pi(a, star, a)));
    show(exprB);
    // Πc:S.Πb:Πb:%P c.Πa:*.a.Πa:*.a
    show(ansnB);
    show(delta_reduce(constant(exprB), delta));
    bout(is_convertible(exprB, ansnB, delta));
    
    exprB = constant("element", {S, x, constant("emptyset", {S})});
    ansnB = constant("contra", {});
    show(exprB);
    show(ansnB);
    bout(is_convertible(exprB, ansnB, delta));

    std::shared_ptr<Term> Bbd = beta_nf(delta_nf(exprB, delta));
    show(Bbd);
    // bout(is_convertible(Bbd, ans, delta));
    exprB = appl(P, y);
    ansnB = constant("element", {S, y, lambda(a, S, appl(P, a))});
    show(exprB);
    show(ansnB);
    bout(is_convertible(exprB, ansnB, delta));

    exprB = parse_lambda("?a:?a:A.or[B, A].?b:?b:B.or[B, A].or[B, A]");
    ansnB = parse_lambda("?d:?c:A.?a:*.?d:?b:B.a.?e:?b:A.a.a.?e:?c:B.?a:*.?e:?b:B.a.?f:?b:A.a.a.?a:*.?c:?b:B.a.?f:?b:A.a.a");
    show(exprB);
    show(ansnB);
    bout(is_convertible(exprB, ansnB, delta));

    exprB = parse_lambda("?d:S.?c:%P a.?g:%P d.?h:?e:S.*.?i:*.?e:?e:?e:%h a.%h d.?j:?j:%h d.%h a.i.i");
    ansnB = parse_lambda("?g:S.?d:%P a.?f:%P g.?h:?c:S.*.?c:*.?e:?e:?e:%h a.%h g.?i:?i:%h g.%h a.c.c");
    show(exprB);
    show(ansnB);
    bout(is_convertible(exprB, ansnB, delta));

    sep();
}

void test_reduction2(const Environment& delta) {
    std::cerr << "[delta reduction test]" << std::endl;
    auto exprB = parse_lambda("?a:A.A");
    auto ansnB = parse_lambda("implies[A, A]");
    bout(is_convertible(exprB, ansnB, delta));
    // bout(is_convertible(ansnB, exprB, delta));
    bout(!is_delta_reducible(exprB, delta));
    bout(is_delta_reducible(ansnB, delta));
    bout(alpha_comp(parse_lambda("A"), parse_lambda("A")));
}

int main() {
    Environment delta;
    try {
        delta = Environment("def_file_bez_cp");
    } catch (ParseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    } catch (TokenizeError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    test_alpha_subst();
    test_subst();
    test_reduction1(delta);
    // test_reduction2(delta);
}
