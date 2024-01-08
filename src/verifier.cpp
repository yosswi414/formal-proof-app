// objective: ./test_book3
// generate a book from a given script

#include <iostream>
#include <sstream>

#include "common.hpp"
#include "lambda.hpp"

int main(int argc, char* argv[]) {
    for(int i = 0; i < argc; ++i) {
        std::cout << "arg #" << i << ":\t[" << argv[i] << "]" << std::endl;
    }

    std::vector<std::string> lines;

    if (argc == 2) {
        lines = read_lines(argv[1]);
    }
    else {
        std::string line;
        while (std::getline(std::cin, line)) lines.emplace_back(line);
    }

    for (int i = 0; i < lines.size(); ++i) {
        std::cout << "line #" << i << ":\t[" << lines[i] << "]" << std::endl;
    }
    Book book;

    std::cout << book.string() << std::endl;
    book.sort();
    std::cout << book.string() << std::endl;
    return 0;

    std::stringstream ss;
    for (int i = 0; i < lines.size(); ++i){
        ss << lines[i];
        int lno;
        std::string op;
        ss >> lno >> op;
        if (op == "sort") {
            book.sort();
        }
        else if (op == "var") {
            int m;
            char x;
            ss >> m >> x;
            book.var(m, x);
        }
        else if (op == "weak") {
            int m, n;
            char x;
            ss >> m >> n >> x;
            book.weak(m, n, x);
        }
        else if (op == "form") {
            int m, n;
            ss >> m >> n;
            book.form(m, n);
        }
    }
}
