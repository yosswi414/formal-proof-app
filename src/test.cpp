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

size_t test_success = 0;
size_t test_fail = 0;

#define test(expr)                                                         \
    do {                                                                   \
        bool v = (expr);                                                   \
        std::cerr << #expr " --> " << (v ? "true" : "false") << std::endl; \
        ++(v ? test_success : test_fail);                                  \
    } while (false)

#define test_result()                                                                                \
    do {                                                                                             \
        sep();                                                                                       \
        std::cerr << "[test result]" << std::endl                                                    \
                  << "success: " << test_success << " / " << (test_success + test_fail) << std::endl \
                  << "   fail: " << test_fail << " / " << (test_success + test_fail) << std::endl;   \
        test_success = test_fail = 0;                                                                \
        sep();                                                                                       \
    } while (false)

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

    test(alpha_comp(l1, l1));
    test(alpha_comp(l1, l2));
    test(alpha_comp(l1, l3));
    test(!alpha_comp(l1, l4));
    test(!alpha_comp(l1, l5));
    test(!alpha_comp(l1, l6));

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

    test(alpha_comp(la1, la2));
    test(alpha_comp(la1, la3));
    test(!alpha_comp(la1, la4));
    test(!alpha_comp(la1, la5));

    sep();

    std::cerr << "[textbook p.12 Example 1.6.4]" << std::endl;

    auto lhs1 = lambda(y, star, appl(y, x));
    auto rhs1 = lambda(z, star, appl(z, appl(x, y)));
    show(lhs1);
    subst_show(lhs1, x, appl(x, y));
    show(rhs1);
    test(alpha_comp(substitute(lhs1, x, appl(x, y)), rhs1));

    sep();

    // (2)
    auto lhs2 = lambda(x, star, appl(y, x));
    auto rhs2 = lambda(z, star, appl(y, z));
    show(lhs2);
    subst_show(lhs2, x, appl(x, y));
    show(rhs2);
    test(alpha_comp(substitute(lhs2, x, appl(x, y)), rhs2));
    test(alpha_comp(substitute(lhs2, x, appl(x, y)), lhs2));

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
    test(alpha_comp(substitute(lhs3, z, y), rhs3_1));
    test(alpha_comp(substitute(lhs3, z, y), rhs3_2));
    test(!alpha_comp(substitute(lhs3, z, y), rhs3_3));

    test_result();
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
    test(alpha_comp(sub, ans));

    test_result();
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
    test(is_convertible(exprB, ansnB, delta));
    
    exprB = constant("element", {S, x, constant("emptyset", {S})});
    ansnB = constant("contra", {});
    show(exprB);
    show(ansnB);
    test(is_convertible(exprB, ansnB, delta));

    std::shared_ptr<Term> Bbd = beta_nf(delta_nf(exprB, delta));
    show(Bbd);
    // bout(is_convertible(Bbd, ans, delta));
    exprB = appl(P, y);
    ansnB = constant("element", {S, y, lambda(a, S, appl(P, a))});
    show(exprB);
    show(ansnB);
    test(is_convertible(exprB, ansnB, delta));

    exprB = parse_lambda("?a:?a:A.or[B, A].?b:?b:B.or[B, A].or[B, A]");
    ansnB = parse_lambda("?d:?c:A.?a:*.?d:?b:B.a.?e:?b:A.a.a.?e:?c:B.?a:*.?e:?b:B.a.?f:?b:A.a.a.?a:*.?c:?b:B.a.?f:?b:A.a.a");
    show(exprB);
    show(ansnB);
    test(is_convertible(exprB, ansnB, delta));

    exprB = parse_lambda("?d:S.?c:%P a.?g:%P d.?h:?e:S.*.?i:*.?e:?e:?e:%h a.%h d.?j:?j:%h d.%h a.i.i");
    ansnB = parse_lambda("?g:S.?d:%P a.?f:%P g.?h:?c:S.*.?c:*.?e:?e:?e:%h a.%h g.?i:?i:%h g.%h a.c.c");
    show(exprB);
    show(ansnB);
    test(is_convertible(exprB, ansnB, delta));

    test_result();
}

void test_reduction2(const Environment& delta) {
    std::cerr << "[delta reduction test 2]" << std::endl;
    auto exprB = parse_lambda("?a:A.A");
    auto ansnB = parse_lambda("implies[A, A]");
    test(is_convertible(exprB, ansnB, delta));
    // bout(is_convertible(ansnB, exprB, delta));
    test(!is_delta_reducible(exprB, delta));
    test(is_delta_reducible(ansnB, delta));
    test(alpha_comp(parse_lambda("A"), parse_lambda("A")));
    test_result();
}

void test_reduction3(const Environment& delta) {
    std::cerr << "[delta reduction test 3]" << std::endl;
    std::shared_ptr<Term> exprB, ansnB;
    
    try{
        exprB = parse_lambda("Rset_fig14-10[A, a, f, g, spec_rec_thD22C[A, a, f, g, e, v, b, u]]");
        ansnB = parse_lambda(
            R"(?q:*.?r:?r:?j:*.?l:?l:?l:?c:integer[].?d:A.*.?c:?c:*.?k:?k:%%l zero[] a.?h:?h:integer[].?m:A.?i:*.?o:?n:?n:?n:*.?o:?o:?o:?d:integer[].*.?d:?d:*.?r:?r:%o zero[].?s:?s:integer[].?t:%o s.%o %Funcs[] s.d.d.%o iota[integer[], $r:integer[].eq[integer[], %Funcs[] r, %Funcs[] h], a12_14-3[%Funcs[] h]].?d:%%l h m.n.n.%%l %Funcs[] h %f m.?o:?o:?o:*.?r:?r:?r:?r:?d:integer[].*.?s:?s:*.?t:?t:%r zero[].?x:?x:integer[].?d:%r x.%r %Funcs[] x.s.s.%r iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14-3[h]].?d:*.d.?d:%%l h m.o.o.%%l iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14-3[h]] %g m.i.i.c.c.%%l zero[] a.?h:?h:?h:*.?d:?d:?d:?c:integer[].*.?c:*.?i:?i:?i:%d zero[].%d %Funcs[] e.?k:?k:%d %Funcs[] e.%d zero[].c.c.?i:?i:?c:A.*.?c:*.?k:?k:?k:%i a.%i b.?m:?m:%i b.%i a.c.c.h.h.?c:*.c.j.j.?s:?s:integer[].?n:A.?t:*.?x:?x:?m:?m:*.?x:?x:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, %Funcs[] s], a12_14-3[%Funcs[] s]].?y:?y:*.?z:?z:?z:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%z zero[] a.?B:?B:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?C:?C:integer[].?D:%i C.%i %Funcs[] C.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] B], a12_14-3[%Funcs[] B]].?c:%%z B k.h.h.%%z %Funcs[] B %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?C:?C:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $C:integer[].eq[integer[], %Funcs[] C, B], a12_14-3[B]].?c:*.c.?c:%%z B k.d.d.%%z iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, B], a12_14-3[B]] %g k.j.j.l.l.%%z s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.y.y.m.m.?x:*.?y:?y:?y:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%y zero[] a.?z:?z:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?B:?B:integer[].?C:%i B.%i %Funcs[] B.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] z], a12_14-3[%Funcs[] z]].?c:%%y z k.h.h.%%y %Funcs[] z %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?B:?B:%h zero[].?C:?C:integer[].?D:%h C.%h %Funcs[] C.c.c.%h iota[integer[], $B:integer[].eq[integer[], %Funcs[] B, z], a12_14-3[z]].?c:*.c.?c:%%y z k.d.d.%%y iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, z], a12_14-3[z]] %g k.j.j.l.l.%%y %Funcs[] s %f n.?h:?h:?h:*.?i:?i:?d:?c:integer[].*.?c:*.?i:?i:?i:%d %Funcs[] s.%d %Funcs[] e.?j:?j:%d %Funcs[] e.%d %Funcs[] s.c.c.?j:?j:?c:A.*.?c:*.?d:?d:?d:%j %f n.%j b.?k:?k:%j b.%j %f n.c.c.h.h.?c:*.c.x.x.?y:?y:?m:*.?y:?y:?h:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, s], a12_14-3[s]].?c:*.c.?z:?z:*.?B:?B:?B:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%B zero[] a.?C:?C:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?D:?D:integer[].?E:%i D.%i %Funcs[] D.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14-3[%Funcs[] C]].?c:%%B C k.h.h.%%B %Funcs[] C %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?D:?D:%h zero[].?E:?E:integer[].?F:%h E.%h %Funcs[] E.c.c.%h iota[integer[], $D:integer[].eq[integer[], %Funcs[] D, C], a12_14-3[C]].?c:*.c.?c:%%B C k.d.d.%%B iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14-3[C]] %g k.j.j.l.l.%%B s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.z.z.m.m.?z:*.?m:?m:?i:?c:integer[].?d:A.*.?B:?B:*.?o:?o:%%i zero[] a.?C:?C:integer[].?l:A.?k:*.?j:?j:?h:?h:*.?j:?j:?j:?c:integer[].*.?m:?m:*.?c:?c:%j zero[].?D:?D:integer[].?d:%j D.%j %Funcs[] D.m.m.%j iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14-3[%Funcs[] C]].?c:%%i C l.h.h.%%i %Funcs[] C %f l.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?m:?m:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $m:integer[].eq[integer[], %Funcs[] m, C], a12_14-3[C]].?c:*.c.?c:%%i C l.d.d.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14-3[C]] %g l.k.k.B.B.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14-3[s]] %g n.?j:?j:?j:*.?i:?i:?i:?c:integer[].*.?d:*.?c:?c:?c:%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14-3[s]].%i %Funcs[] e.?h:?h:%i %Funcs[] e.%i iota[integer[], $k:integer[].eq[integer[], %Funcs[] k, s], a12_14-3[s]].d.d.?c:?c:?c:A.*.?d:*.?h:?h:?h:%c %g n.%c b.?k:?k:%c b.%c %g n.d.d.j.j.?c:*.c.z.z.t.t.q.q)");
    } catch (ExprError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    show(exprB);
    show(ansnB);
    std::cerr << "FV(exprB) = " << to_string(free_var(exprB)) << std::endl;
    std::cerr << "FV(ansnB) = " << to_string(free_var(ansnB)) << std::endl;
    auto eBdnf = delta_nf(exprB, delta);
    show(eBdnf);


    test(is_convertible(exprB, ansnB, delta));
    
    test_result();
}

int main() {
    Environment delta;
    try {
        delta = Environment("def_file_bez_438259_cp");
    } catch (ParseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    } catch (TokenizeError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    // test_alpha_subst();
    // test_subst();
    // test_reduction1(delta);
    // test_reduction2(delta);
    test_reduction3(delta);
}
