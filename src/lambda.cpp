#include "lambda.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

int main() {
    std::cout << Kind::Variable << std::endl;
    // Term* ts = new Star();
    std::shared_ptr<Term> ts = std::make_shared<Star>();
    std::shared_ptr<Term> ts2 = std::make_shared<Square>();
    std::cout << ts << " " << ts2 << std::endl;
    std::shared_ptr<Term> ts3 = std::make_shared<Application>(ts, ts2);
    std::cout << ts3 << std::endl;
    std::shared_ptr<Term> ts4 = std::make_shared<Application>(ts3, ts);
    std::cout << ts4 << std::endl;
    std::cout << std::is_pointer<Term*>::value << std::endl;
    std::cout << std::is_pointer<std::shared_ptr<Term>>::value << std::endl;
    std::shared_ptr<Typed<Variable>> tv = std::make_shared<Typed<Variable>>(std::make_shared<Variable>('x'),
                                                                            std::make_shared<Application>(
                                                                                std::make_shared<Star>(),
                                                                                std::make_shared<Star>()));
    std::cout << tv << std::endl;

    std::shared_ptr<Term> tlambda = std::make_shared<AbstLambda>(
        std::make_shared<Typed<Variable>>(std::make_shared<Variable>('x'), std::make_shared<Variable>('A')),
        std::make_shared<Variable>('B'));

    std::cout << "lambda ... " << tlambda << std::endl;

    std::shared_ptr<Term> tconst = std::make_shared<Constant>(
        "hello",
        ts,
        ts3,
        ts2);

    std::cout << "constant ... " << tconst << std::endl;

    std::shared_ptr<Term> tcon0 = std::make_shared<Constant>("world");

    std::cout << "constant ... " << tcon0 << std::endl;

    std::shared_ptr<Term>
        x(std::make_shared<Variable>('x')),
        y(std::make_shared<Variable>('y')),
        z(std::make_shared<Variable>('z')),
        A(std::make_shared<Variable>('A')),
        B(std::make_shared<Variable>('B')),
        C(std::make_shared<Variable>('C')),
        star(std::make_shared<Star>()),
        sq(std::make_shared<Square>());
    // std::cout << x << y << A << B << star << sq << std::endl;

    std::shared_ptr<Typed<Variable>>
        xA(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(x), A)),
        yB(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(y), B)),
        zC(std::make_shared<Typed<Variable>>(std::dynamic_pointer_cast<Variable>(z), C));
    // std::cout << xA << std::endl;

    std::shared_ptr<Term>
        PxAB(std::make_shared<AbstPi>(xA, B)),
        PxABz(std::make_shared<Application>(PxAB, z));

    // std::cout << PxABz << std::endl;

    std::shared_ptr<Context>
        conxy(std::make_shared<Context>(xA, yB)),
        conyz(std::make_shared<Context>(yB, zC)),
        conzx(std::make_shared<Context>(zC, xA));

    std::shared_ptr<Constant>
        const1(std::make_shared<Constant>("hoge", A, B)),
        const2(std::make_shared<Constant>("fuga", B, C)),
        const3(std::make_shared<Constant>("piyo", C, A));

    std::shared_ptr<Definition>
        defpr1(std::make_shared<Definition>(conxy, const2, PxAB)),
        defds(std::make_shared<Definition>(conzx, const1, PxABz, PxAB)),
        defpr2(std::make_shared<Definition>(conyz, const3, PxABz));

    std::shared_ptr<Environment>
        defs1(std::make_shared<Environment>(defds, defpr1)),
        defs2(std::make_shared<Environment>(defpr2, defds));

    // std::cout << defs1 << std::endl;

    std::shared_ptr<Judgement>
        judge1(std::make_shared<Judgement>(defs1, conyz, PxAB, star)),
        judge2(std::make_shared<Judgement>(defs2, conxy, star, sq));

    std::shared_ptr<Book>
        book(std::make_shared<Book>(judge1, judge2));

    std::cout << "\n\n";
    std::cout << "defpr1 = " << defpr1 << std::endl;
    std::cout << "defpr2 = " << defpr2 << std::endl;
    std::cout << "defds  = " << defds << std::endl;
    std::cout << "\n";
    std::cout << "env defs1 = " << defs1 << std::endl;
    std::cout << " = \n";
    std::cout << defs1->repr() << std::endl;
    std::cout << "\n";
    std::cout << "judge judge1 = " << judge1 << std::endl;
    std::cout << "judge judge2 = " << judge2->string(false) << std::endl;
    std::cout << "\n";
    std::cout << book << std::endl;

    std::vector<std::string> lines{
        "def2   // (123)",
        "1",
        "A",
        "*",
        "test   // name",
        "$x:(A).(*)",
        "*",
        "edef2",
        "",
        "END"
    };


}
