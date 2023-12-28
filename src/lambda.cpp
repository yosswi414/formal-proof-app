#include <iostream>

#include "lambda.hpp"

int main(){
    Variable var('x');
    Star star;
    Square sq;
    Lambda expr;
    std::cout << "type: [" << expr.typestr() << "]" << std::endl;
    expr = Star();
    std::cout << "type: [" << expr.typestr() << "]" << std::endl;
    expr = Square();
    std::cout << "type: [" << expr.typestr() << "]" << std::endl;
    expr = Variable('y');
    std::cout << "type: [" << expr.typestr() << "]" << std::endl;
    std::cout << "type: [" << var.typestr() << "]" << std::endl;
}
