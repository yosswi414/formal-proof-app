#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

#include "book.hpp"
#include "context.hpp"
#include "environment.hpp"
#include "inference.hpp"
#include "lambda.hpp"
#include "parser.hpp"

bool bout_result;

#define bout(expr) \
    do { std::cerr << #expr " --> " << ((bout_result = (expr)) ? "true" : "false") << std::endl; } while (false)
#define show(expr) \
    do { std::cerr << #expr " = " << expr << std::endl; } while (false)
#define subst_show(e, p, q) \
    do { std::cerr << "(" << e << ")[" << p << ":=" << q << "] --> " << substitute(e, p, q) << std::endl; } while (false);
#define sep() \
    do { std::cerr << "####################" << std::endl; } while (false)

size_t test_success = 0;
size_t test_fail = 0;

#define STR_SUCCESS BOLD(GREEN("OK"))
#define STR_FAIL BOLD(RED("NG"))

#define test(expr)                                                               \
    do {                                                                         \
        bool v = (expr);                                                         \
        std::cerr << #expr " --> " << (v ? STR_SUCCESS : STR_FAIL) << std::endl; \
        ++(v ? test_success : test_fail);                                        \
    } while (false)

#define cshow(a, ...)                                                                              \
    do {                                                                                           \
        std::cerr << #a ", " #__VA_ARGS__ << " |> " << beta_nf(appl(a, __VA_ARGS__)) << std::endl; \
    } while (false)

#define btest(from, to)                                                                     \
    do {                                                                                    \
        bool v = alpha_comp(beta_nf(from), to);                                             \
        std::cerr << #from " |> " #to " --> " << (v ? STR_SUCCESS : STR_FAIL) << std::endl; \
        ++(v ? test_success : test_fail);                                                   \
    } while (false)

#define test_result()                                                                                \
    do {                                                                                             \
        sep();                                                                                       \
        std::cerr << "[test result]" << std::endl                                                    \
                  << "success: " << test_success << " / " << (test_success + test_fail) << std::endl \
                  << "   fail: " << test_fail << " / " << (test_success + test_fail) << std::endl;   \
        sep();                                                                                       \
        if (test_fail > 0) exit(EXIT_FAILURE);                                                       \
        test_success = test_fail = 0;                                                                \
    } while (false)

// #define defvar(vname) std::shared_ptr<Term> vname = variable(#vname[0])
#define defvar(vname) std::shared_ptr<Term> vname = variable(#vname)

void test_alpha_subst() {
    std::shared_ptr<Term> e1, e2, e3;
    defvar(x);
    defvar(y);
    defvar(z);
    defvar(u);
    defvar(v);

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

    auto la1 = lambda(x, star, lambda(y, star, appl(x, z, y)));
    auto la2 = lambda(v, star, lambda(y, star, appl(v, z, y)));
    auto la3 = lambda(v, star, lambda(u, star, appl(v, z, u)));
    auto la4 = lambda(y, star, lambda(y, star, appl(y, z, y)));
    auto la5 = lambda(z, star, lambda(y, star, appl(z, z, y)));

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
    auto lhs3 = lambda(x, star, lambda(y, star, appl(z, z, x)));
    auto rhs3_1 = lambda(u, star, lambda(v, star, appl(y, y, u)));
    auto rhs3_2 = lambda(x, star, lambda(v, star, appl(y, y, x)));
    auto rhs3_3 = lambda(x, star, lambda(y, star, appl(y, y, x)));
    show(lhs3);
    subst_show(lhs3, z, y);
    auto lhs3_subst = substitute(lhs3, z, y);
    show(lhs3_subst);
    show(free_var(lhs3_subst));
    show(rhs3_1);
    show(free_var(rhs3_1));
    show(rhs3_2);
    show(free_var(rhs3_2));
    show(rhs3_3);
    show(free_var(rhs3_3));

    test(alpha_comp(lhs3_subst, rhs3_1));
    test(alpha_comp(lhs3_subst, rhs3_2));
    test(!alpha_comp(lhs3_subst, rhs3_3));

    test_result();
}

void test_subst() {
    std::cerr << "[substitution test]" << std::endl;
    defvar(A);
    defvar(a);
    defvar(b);
    defvar(c);
    defvar(d);
    defvar(e);
    defvar(f);

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

void test_sandbox_combinators() {
    std::cerr << "[combinator test]" << std::endl;

    defvar(x);
    defvar(y);
    defvar(z);

    using Expr = std::shared_ptr<Term>;

    // untyped abstraction λv.e
    auto abst = [](const std::shared_ptr<Term>& v, const Expr& e) { return lambda(v, star, e); };

    // identity; I(f) = f
    Expr I = abst(x, x);
    // constant; K(c)(x) = c
    Expr K = abst(x, abst(y, x));
    // stronger composition; S(f, g)(x) = f(x, g(x))
    Expr S = abst(x, abst(y, abst(z, appl(x, z, appl(y, z)))));

    // basic combinators
    show(I);
    show(K);
    show(S);

    // weak reduction
    btest(appl(I, x), x);
    btest(appl(K, x, y), x);
    btest(appl(S, x, y, z), appl(x, z, appl(y, z)));

    // composition; B(f, g)(x) = f(g(x))
    Expr B = appl(S, appl(K, S), K);
    btest(appl(B, x, y, z), appl(x, appl(y, z)));

    // commutativity; C(f)(x, y) = f(y, x)
    Expr C = appl(S, appl(B, B, S), appl(K, K));
    btest(appl(C, x, y, z), appl(x, z, y));

    // diagonalizer; W(f)(x) = f(x, x)
    Expr W = appl(S, S, appl(K, I));
    btest(appl(W, x, y), appl(x, y, y));

    Expr I2 = appl(W, K);
    Expr S2 = appl(B, appl(B, appl(B, W), C), appl(B, B));
    Expr S3 = appl(B, appl(B, W), appl(B, B, C));
    btest(appl(I2, x), x);
    btest(appl(S2, x, y, z), appl(x, z, appl(y, z)));
    btest(appl(S3, x, y, z), appl(x, z, appl(y, z)));

    // swap; M(x, y) = (y, x)
    Expr M = appl(C, I);
    btest(appl(M, x, y), appl(y, x));

    Expr M2 = appl(S, appl(K, appl(S, I)), K);
    btest(appl(M2, x, y), appl(y, x));

    // irreducible
    // Expr irr = appl(S, I, I, appl(S, I, I));
    // cshow(irr);

    test_result();
}

void test_reduction1(const Environment& delta) {
    std::cerr << "[beta / delta reduction test]" << std::endl;
    defvar(S);
    defvar(P);
    defvar(v);
    defvar(a);
    defvar(b);
    defvar(c);
    defvar(d);
    defvar(e);
    defvar(f);
    defvar(g);
    defvar(h);
    defvar(i);
    defvar(j);
    defvar(x);
    defvar(y);
    defvar(A);
    defvar(B);

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
    exprB = parse_lambda("?a:?a:A.or[B, A].?b:?b:B.or[B, A].or[B, A]", delta);
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
    auto ansnB = parse_lambda("implies[A, A]", delta);
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

    try {
        exprB = parse_lambda("Rset_fig14.10[A, a, f, g, spec_rec_thD22C[A, a, f, g, e, v, b, u]]", delta);
        show(exprB);
        ansnB = parse_lambda(
            R"(?q:*.?r:?r:?j:*.?l:?l:?l:?c:integer[].?d:A.*.?c:?c:*.?k:?k:%%l zero[] a.?h:?h:integer[].?m:A.?i:*.?o:?n:?n:?n:*.?o:?o:?o:?d:integer[].*.?d:?d:*.?r:?r:%o zero[].?s:?s:integer[].?t:%o s.%o %Funcs[] s.d.d.%o iota[integer[], $r:integer[].eq[integer[], %Funcs[] r, %Funcs[] h], a12_14.3[%Funcs[] h]].?d:%%l h m.n.n.%%l %Funcs[] h %f m.?o:?o:?o:*.?r:?r:?r:?r:?d:integer[].*.?s:?s:*.?t:?t:%r zero[].?x:?x:integer[].?d:%r x.%r %Funcs[] x.s.s.%r iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14.3[h]].?d:*.d.?d:%%l h m.o.o.%%l iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14.3[h]] %g m.i.i.c.c.%%l zero[] a.?h:?h:?h:*.?d:?d:?d:?c:integer[].*.?c:*.?i:?i:?i:%d zero[].%d %Funcs[] e.?k:?k:%d %Funcs[] e.%d zero[].c.c.?i:?i:?c:A.*.?c:*.?k:?k:?k:%i a.%i b.?m:?m:%i b.%i a.c.c.h.h.?c:*.c.j.j.?s:?s:integer[].?n:A.?t:*.?x:?x:?m:?m:*.?x:?x:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, %Funcs[] s], a12_14.3[%Funcs[] s]].?y:?y:*.?z:?z:?z:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%z zero[] a.?B:?B:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?C:?C:integer[].?D:%i C.%i %Funcs[] C.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] B], a12_14.3[%Funcs[] B]].?c:%%z B k.h.h.%%z %Funcs[] B %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?C:?C:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $C:integer[].eq[integer[], %Funcs[] C, B], a12_14.3[B]].?c:*.c.?c:%%z B k.d.d.%%z iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, B], a12_14.3[B]] %g k.j.j.l.l.%%z s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.y.y.m.m.?x:*.?y:?y:?y:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%y zero[] a.?z:?z:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?B:?B:integer[].?C:%i B.%i %Funcs[] B.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] z], a12_14.3[%Funcs[] z]].?c:%%y z k.h.h.%%y %Funcs[] z %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?B:?B:%h zero[].?C:?C:integer[].?D:%h C.%h %Funcs[] C.c.c.%h iota[integer[], $B:integer[].eq[integer[], %Funcs[] B, z], a12_14.3[z]].?c:*.c.?c:%%y z k.d.d.%%y iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, z], a12_14.3[z]] %g k.j.j.l.l.%%y %Funcs[] s %f n.?h:?h:?h:*.?i:?i:?d:?c:integer[].*.?c:*.?i:?i:?i:%d %Funcs[] s.%d %Funcs[] e.?j:?j:%d %Funcs[] e.%d %Funcs[] s.c.c.?j:?j:?c:A.*.?c:*.?d:?d:?d:%j %f n.%j b.?k:?k:%j b.%j %f n.c.c.h.h.?c:*.c.x.x.?y:?y:?m:*.?y:?y:?h:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, s], a12_14.3[s]].?c:*.c.?z:?z:*.?B:?B:?B:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%B zero[] a.?C:?C:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?D:?D:integer[].?E:%i D.%i %Funcs[] D.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14.3[%Funcs[] C]].?c:%%B C k.h.h.%%B %Funcs[] C %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?D:?D:%h zero[].?E:?E:integer[].?F:%h E.%h %Funcs[] E.c.c.%h iota[integer[], $D:integer[].eq[integer[], %Funcs[] D, C], a12_14.3[C]].?c:*.c.?c:%%B C k.d.d.%%B iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14.3[C]] %g k.j.j.l.l.%%B s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.z.z.m.m.?z:*.?m:?m:?i:?c:integer[].?d:A.*.?B:?B:*.?o:?o:%%i zero[] a.?C:?C:integer[].?l:A.?k:*.?j:?j:?h:?h:*.?j:?j:?j:?c:integer[].*.?m:?m:*.?c:?c:%j zero[].?D:?D:integer[].?d:%j D.%j %Funcs[] D.m.m.%j iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14.3[%Funcs[] C]].?c:%%i C l.h.h.%%i %Funcs[] C %f l.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?m:?m:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $m:integer[].eq[integer[], %Funcs[] m, C], a12_14.3[C]].?c:*.c.?c:%%i C l.d.d.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14.3[C]] %g l.k.k.B.B.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14.3[s]] %g n.?j:?j:?j:*.?i:?i:?i:?c:integer[].*.?d:*.?c:?c:?c:%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14.3[s]].%i %Funcs[] e.?h:?h:%i %Funcs[] e.%i iota[integer[], $k:integer[].eq[integer[], %Funcs[] k, s], a12_14.3[s]].d.d.?c:?c:?c:A.*.?d:*.?h:?h:?h:%c %g n.%c b.?k:?k:%c b.%c %g n.d.d.j.j.?c:*.c.z.z.t.t.q.q)",
            delta);
        show(ansnB);
    } catch (ExprError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    } catch (ParseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    // show(ansnB->repr_new());
    std::cerr << "FV(exprB) = " << to_string(free_var(exprB)) << std::endl;
    std::cerr << "FV(ansnB) = " << to_string(free_var(ansnB)) << std::endl;
    std::cerr << "test 0" << std::endl;
    bout(free_var(exprB) == free_var(ansnB));
    // if (!bout_result) return;
    std::cerr << "test 1" << std::endl;
    test(is_convertible(exprB, ansnB, delta));
    test(!alpha_comp(exprB, ansnB));
    std::cerr << "test 2" << std::endl;
    while (true) {
        int rank_e = expr_rank(exprB, delta);
        int rank_a = expr_rank(ansnB, delta);
        show(rank_e);
        show(rank_a);
        bout(alpha_comp(exprB, ansnB));
        bout(is_convertible(exprB, ansnB, delta));
        auto exprB_n = NF_above(exprB, delta, std::min(rank_e, rank_a));
        auto ansnB_n = NF_above(ansnB, delta, std::min(rank_e, rank_a));
        bool brk = false;
        bout(alpha_comp(exprB, exprB_n));
        if (bout_result) brk = true;
        bout(alpha_comp(ansnB, ansnB_n));
        if (bout_result && brk) break;
        exprB = exprB_n;
        ansnB = ansnB_n;
    }
    std::cerr << "test 3" << std::endl;
    show(expr_rank(NF(exprB, delta), delta));
    show(expr_rank(NF(ansnB, delta), delta));
    test(is_convertible(exprB, ansnB, delta));
    test(alpha_comp(exprB, ansnB));

    test_result();
}

void test_parse(const Environment& delta) {
    std::cerr << "[parse test]" << std::endl;
    std::shared_ptr<Term> A;

    try {
        A = parse_lambda("Rset_fig14.10[A, a, f, g, spec_rec_thD22C[A, a, f, g, e, v, b, u]]", delta);
    } catch (ExprError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    show(A);
}

void test_def_file(const Environment& delta) {
    std::shared_ptr<Term> a, b;
    try {
        a = parse_lambda(R"(?a:?a:T.*.implies[%a %f x, %a %f y])", delta);
        b = parse_lambda(R"(eq[T, %f x, %f y])", delta);
    } catch (ParseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    } catch (TokenizeError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }

    show(a);
    show(b);
    bout(is_convertible(a, b, delta));
    auto ae = NF(a, delta);
    auto be = NF(b, delta);
    show(ae);
    show(be);
    bout(is_convertible(ae, be, delta));
}

void test_get_type(const Book& book) {
    for (size_t i = 0; i < book.size(); ++i) {
        std::shared_ptr<Term> M = book[i].term();
        std::shared_ptr<Environment> delta = book[i].env();
        std::shared_ptr<Context> gamma = book[i].context();
        std::shared_ptr<Term> T = get_type(M, delta, gamma);
        std::cerr << "[" << i << "] M = " << M << " : " << book[i].type() << std::endl;
        test(T && is_convertible(T, book[i].type(), *delta));
    }

    test_result();
}

// void test_parse2(const Environment& delta) {
//     try {
//         // {
//         //     std::string test_expr_1_str = "?a:*.?b:?c:?d:*.?e:?f:?g:?h:integer[].?i:A.*.T1.T2.T3.T4.T5";
//         //     auto test_expr_1_lam = parse_lambda(test_expr_1_str);
//         //     show(test_expr_1_str);
//         //     show(test_expr_1_lam);
//         //     show(test_expr_1_lam->repr());
//         // }
//         {
//             std::string test_expr_3_str = R"(?q:*.?r:?r:?j:*.?l:?l:?l:?c:integer[].?d:A.*.?c:?c:*.?k:?k:%%l zero[] a.?h:?h:integer[].?m:A.?i:*.?o:?n:?n:?n:*.?o:?o:?o:?d:integer[].*.?d:?d:*.?r:?r:%o zero[].?s:?s:integer[].?t:%o s.%o %Funcs[] s.d.d.%o iota[integer[], $r:integer[].eq[integer[], %Funcs[] r, %Funcs[] h], a12_14.3[%Funcs[] h]].?d:%%l h m.n.n.%%l %Funcs[] h %f m.?o:?o:?o:*.?r:?r:?r:?r:?d:integer[].*.?s:?s:*.?t:?t:%r zero[].?x:?x:integer[].?d:%r x.%r %Funcs[] x.s.s.%r iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14.3[h]].?d:*.d.?d:%%l h m.o.o.%%l iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14.3[h]] %g m.i.i.c.c.%%l zero[] a.?h:?h:?h:*.?d:?d:?d:?c:integer[].*.?c:*.?i:?i:?i:%d zero[].%d %Funcs[] e.?k:?k:%d %Funcs[] e.%d zero[].c.c.?i:?i:?c:A.*.?c:*.?k:?k:?k:%i a.%i b.?m:?m:%i b.%i a.c.c.h.h.?c:*.c.j.j.?s:?s:integer[].?n:A.?t:*.?x:?x:?m:?m:*.?x:?x:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, %Funcs[] s], a12_14.3[%Funcs[] s]].?y:?y:*.?z:?z:?z:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%z zero[] a.?B:?B:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?C:?C:integer[].?D:%i C.%i %Funcs[] C.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] B], a12_14.3[%Funcs[] B]].?c:%%z B k.h.h.%%z %Funcs[] B %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?C:?C:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $C:integer[].eq[integer[], %Funcs[] C, B], a12_14.3[B]].?c:*.c.?c:%%z B k.d.d.%%z iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, B], a12_14.3[B]] %g k.j.j.l.l.%%z s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.y.y.m.m.?x:*.?y:?y:?y:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%y zero[] a.?z:?z:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?B:?B:integer[].?C:%i B.%i %Funcs[] B.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] z], a12_14.3[%Funcs[] z]].?c:%%y z k.h.h.%%y %Funcs[] z %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?B:?B:%h zero[].?C:?C:integer[].?D:%h C.%h %Funcs[] C.c.c.%h iota[integer[], $B:integer[].eq[integer[], %Funcs[] B, z], a12_14.3[z]].?c:*.c.?c:%%y z k.d.d.%%y iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, z], a12_14.3[z]] %g k.j.j.l.l.%%y %Funcs[] s %f n.?h:?h:?h:*.?i:?i:?d:?c:integer[].*.?c:*.?i:?i:?i:%d %Funcs[] s.%d %Funcs[] e.?j:?j:%d %Funcs[] e.%d %Funcs[] s.c.c.?j:?j:?c:A.*.?c:*.?d:?d:?d:%j %f n.%j b.?k:?k:%j b.%j %f n.c.c.h.h.?c:*.c.x.x.?y:?y:?m:*.?y:?y:?h:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, s], a12_14.3[s]].?c:*.c.?z:?z:*.?B:?B:?B:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%B zero[] a.?C:?C:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?D:?D:integer[].?E:%i D.%i %Funcs[] D.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14.3[%Funcs[] C]].?c:%%B C k.h.h.%%B %Funcs[] C %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?D:?D:%h zero[].?E:?E:integer[].?F:%h E.%h %Funcs[] E.c.c.%h iota[integer[], $D:integer[].eq[integer[], %Funcs[] D, C], a12_14.3[C]].?c:*.c.?c:%%B C k.d.d.%%B iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14.3[C]] %g k.j.j.l.l.%%B s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.z.z.m.m.?z:*.?m:?m:?i:?c:integer[].?d:A.*.?B:?B:*.?o:?o:%%i zero[] a.?C:?C:integer[].?l:A.?k:*.?j:?j:?h:?h:*.?j:?j:?j:?c:integer[].*.?m:?m:*.?c:?c:%j zero[].?D:?D:integer[].?d:%j D.%j %Funcs[] D.m.m.%j iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14.3[%Funcs[] C]].?c:%%i C l.h.h.%%i %Funcs[] C %f l.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?m:?m:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $m:integer[].eq[integer[], %Funcs[] m, C], a12_14.3[C]].?c:*.c.?c:%%i C l.d.d.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14.3[C]] %g l.k.k.B.B.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14.3[s]] %g n.?j:?j:?j:*.?i:?i:?i:?c:integer[].*.?d:*.?c:?c:?c:%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14.3[s]].%i %Funcs[] e.?h:?h:%i %Funcs[] e.%i iota[integer[], $k:integer[].eq[integer[], %Funcs[] k, s], a12_14.3[s]].d.d.?c:?c:?c:A.*.?d:*.?h:?h:?h:%c %g n.%c b.?k:?k:%c b.%c %g n.d.d.j.j.?c:*.c.z.z.t.t.q.q)";
//             auto test_expr_3_lam = parse_lambda(test_expr_3_str, delta);
//             show(test_expr_3_str);
//             // show(test_expr_3_lam);
//             std::string test_expr_3_prs = test_expr_3_lam->repr_new();
//             show(test_expr_3_prs);
//             for (size_t i = 0; i < std::min(test_expr_3_str.size(), test_expr_3_prs.size()); ++i) {
//                 if (test_expr_3_str[i] != test_expr_3_prs[i]) {
//                     show(test_expr_3_str.substr(0, i));
//                     show(test_expr_3_prs.substr(0, i));
//                     break;
//                 }
//             }
//             std::cerr << "FV(expr_3) = " << to_string(free_var(test_expr_3_lam)) << std::endl;
//         }

//         // {
//         //     std::string test_expr_4_str = "?x.a.:?y.b.:var1.var2.var3.var4.var5";
//         //     auto test_expr_4_lam = parse_lambda(test_expr_4_str);
//         //     show(test_expr_4_str);
//         //     show(test_expr_4_lam);
//         //     show(test_expr_4_lam->repr());
//         // }
//     } catch (ExprError& e) {
//         // e.puterror();
//         std::cerr << "[error] aborting." << std::endl;
//     }
// }

void test_new_parser(const Environment& delta) {
    try {
        if (false) {
            std::string test_expr_1_str = "?a:*.?b:?c:?d:*.?e:?f:?g:?h:contra[].?i:A.*.T1.T2.T3.T4.T5";
            auto test_expr_1_lam = parse_lambda(test_expr_1_str, delta);
            show(test_expr_1_str);
            show(test_expr_1_lam);
            show(test_expr_1_lam->repr());
        }
        if (true) {
            std::string test_expr_3_str = R"(?q:*.?r:?r:?j:*.?l:?l:?l:?c:integer[].?d:A.*.?c:?c:*.?k:?k:%%l zero[] a.?h:?h:integer[].?m:A.?i:*.?o:?n:?n:?n:*.?o:?o:?o:?d:integer[].*.?d:?d:*.?r:?r:%o zero[].?s:?s:integer[].?t:%o s.%o %Funcs[] s.d.d.%o iota[integer[], $r:integer[].eq[integer[], %Funcs[] r, %Funcs[] h], a12_14.3[%Funcs[] h]].?d:%%l h m.n.n.%%l %Funcs[] h %f m.?o:?o:?o:*.?r:?r:?r:?r:?d:integer[].*.?s:?s:*.?t:?t:%r zero[].?x:?x:integer[].?d:%r x.%r %Funcs[] x.s.s.%r iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14.3[h]].?d:*.d.?d:%%l h m.o.o.%%l iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, h], a12_14.3[h]] %g m.i.i.c.c.%%l zero[] a.?h:?h:?h:*.?d:?d:?d:?c:integer[].*.?c:*.?i:?i:?i:%d zero[].%d %Funcs[] e.?k:?k:%d %Funcs[] e.%d zero[].c.c.?i:?i:?c:A.*.?c:*.?k:?k:?k:%i a.%i b.?m:?m:%i b.%i a.c.c.h.h.?c:*.c.j.j.?s:?s:integer[].?n:A.?t:*.?x:?x:?m:?m:*.?x:?x:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, %Funcs[] s], a12_14.3[%Funcs[] s]].?y:?y:*.?z:?z:?z:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%z zero[] a.?B:?B:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?C:?C:integer[].?D:%i C.%i %Funcs[] C.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] B], a12_14.3[%Funcs[] B]].?c:%%z B k.h.h.%%z %Funcs[] B %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?C:?C:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $C:integer[].eq[integer[], %Funcs[] C, B], a12_14.3[B]].?c:*.c.?c:%%z B k.d.d.%%z iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, B], a12_14.3[B]] %g k.j.j.l.l.%%z s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.y.y.m.m.?x:*.?y:?y:?y:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%y zero[] a.?z:?z:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?B:?B:integer[].?C:%i B.%i %Funcs[] B.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] z], a12_14.3[%Funcs[] z]].?c:%%y z k.h.h.%%y %Funcs[] z %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?B:?B:%h zero[].?C:?C:integer[].?D:%h C.%h %Funcs[] C.c.c.%h iota[integer[], $B:integer[].eq[integer[], %Funcs[] B, z], a12_14.3[z]].?c:*.c.?c:%%y z k.d.d.%%y iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, z], a12_14.3[z]] %g k.j.j.l.l.%%y %Funcs[] s %f n.?h:?h:?h:*.?i:?i:?d:?c:integer[].*.?c:*.?i:?i:?i:%d %Funcs[] s.%d %Funcs[] e.?j:?j:%d %Funcs[] e.%d %Funcs[] s.c.c.?j:?j:?c:A.*.?c:*.?d:?d:?d:%j %f n.%j b.?k:?k:%j b.%j %f n.c.c.h.h.?c:*.c.x.x.?y:?y:?m:*.?y:?y:?h:?h:?c:integer[].*.?c:?c:*.?i:?i:%h zero[].?j:?j:integer[].?d:%h j.%h %Funcs[] j.c.c.%h iota[integer[], $d:integer[].eq[integer[], %Funcs[] d, s], a12_14.3[s]].?c:*.c.?z:?z:*.?B:?B:?B:?c:integer[].?d:A.*.?l:?l:*.?o:?o:%%B zero[] a.?C:?C:integer[].?k:A.?j:*.?i:?i:?h:?h:*.?i:?i:?i:?c:integer[].*.?d:?d:*.?c:?c:%i zero[].?D:?D:integer[].?E:%i D.%i %Funcs[] D.d.d.%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14.3[%Funcs[] C]].?c:%%B C k.h.h.%%B %Funcs[] C %f k.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?D:?D:%h zero[].?E:?E:integer[].?F:%h E.%h %Funcs[] E.c.c.%h iota[integer[], $D:integer[].eq[integer[], %Funcs[] D, C], a12_14.3[C]].?c:*.c.?c:%%B C k.d.d.%%B iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14.3[C]] %g k.j.j.l.l.%%B s n.?d:?d:?d:*.?i:?h:?c:?c:integer[].*.?h:*.?i:?i:?i:%c s.%c %Funcs[] e.?j:?j:%c %Funcs[] e.%c s.h.h.?i:?i:?c:A.*.?j:*.?c:?c:?c:%i n.%i b.?k:?k:%i b.%i n.j.j.d.d.?c:*.c.z.z.m.m.?z:*.?m:?m:?i:?c:integer[].?d:A.*.?B:?B:*.?o:?o:%%i zero[] a.?C:?C:integer[].?l:A.?k:*.?j:?j:?h:?h:*.?j:?j:?j:?c:integer[].*.?m:?m:*.?c:?c:%j zero[].?D:?D:integer[].?d:%j D.%j %Funcs[] D.m.m.%j iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, %Funcs[] C], a12_14.3[%Funcs[] C]].?c:%%i C l.h.h.%%i %Funcs[] C %f l.?d:?d:?d:*.?h:?h:?h:?h:?c:integer[].*.?c:?c:*.?m:?m:%h zero[].?D:?D:integer[].?E:%h D.%h %Funcs[] D.c.c.%h iota[integer[], $m:integer[].eq[integer[], %Funcs[] m, C], a12_14.3[C]].?c:*.c.?c:%%i C l.d.d.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, C], a12_14.3[C]] %g l.k.k.B.B.%%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14.3[s]] %g n.?j:?j:?j:*.?i:?i:?i:?c:integer[].*.?d:*.?c:?c:?c:%i iota[integer[], $c:integer[].eq[integer[], %Funcs[] c, s], a12_14.3[s]].%i %Funcs[] e.?h:?h:%i %Funcs[] e.%i iota[integer[], $k:integer[].eq[integer[], %Funcs[] k, s], a12_14.3[s]].d.d.?c:?c:?c:A.*.?d:*.?h:?h:?h:%c %g n.%c b.?k:?k:%c b.%c %g n.d.d.j.j.?c:*.c.z.z.t.t.q.q)";
            auto test_expr_3_lam = parse_lambda(test_expr_3_str, delta);
            // show(test_expr_3_str);
            // show(test_expr_3_lam);
            std::string test_expr_3_prs = test_expr_3_lam->repr_new();
            // show(test_expr_3_prs);
            bool mismatch = false;
            for (size_t i = 0; i < std::min(test_expr_3_str.size(), test_expr_3_prs.size()); ++i) {
                if (test_expr_3_str[i] != test_expr_3_prs[i]) {
                    show(test_expr_3_str.substr(0, i));
                    show(test_expr_3_prs.substr(0, i));
                    mismatch = true;
                    break;
                }
            }
            bout(mismatch);
            test(test_expr_3_str == test_expr_3_prs);
            std::cerr << "FV(expr_3) = " << to_string(free_var(test_expr_3_lam)) << std::endl;
        }

        if (false) {
            std::string test_expr_4_str = "?x.a.:?y.b.:var1.var2.var3.var4.var5";
            auto test_expr_4_lam = parse_lambda(test_expr_4_str, delta);
            show(test_expr_4_str);
            show(test_expr_4_lam);
            show(test_expr_4_lam->repr());
        }
    } catch (ExprError& e) {
        // e.puterror();
        std::cerr << "[error] aborting." << std::endl;
    } catch (ParseError& e) {
        e.puterror();
        std::cerr << "[error] aborting." << std::endl;
    }
}

int main() {
    std::vector<Environment> envs;
    try {
        {
            std::vector<std::string> fnames{
                "resource/def_file_bez",
                "resource/def_file"};
            for(auto&& fname : fnames) {
                std::cout << "loading " << fname << "..." << std::flush;
                envs.emplace_back(fname);
                std::cout << " done." << std::endl;
            }
        }

        std::string script = "resource/script_test";
        std::cout << "loading " << script << "..." << std::flush;
        Book book(script);
        std::cout << " done." << std::endl;

        test_alpha_subst();
        test_subst();
        test_reduction1(envs[0]);
        test_reduction2(envs[0]);
        test_sandbox_combinators();

        test_parse(envs[0]);
        test_reduction3(envs[0]);
        test_def_file(envs[1]);

        test_get_type(book);

        // test_parse2(envs[0]);
        test_new_parser(envs[0]);
    } catch (ParseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    } catch (TokenizeError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
}
