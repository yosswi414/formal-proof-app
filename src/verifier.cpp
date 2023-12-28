// objective: ./test_book3
// generate a book from a given script

#include <iostream>

#include "common.hpp"

int main(int argc, char* argv[]) {
    for(int i = 0; i < argc; ++i) {
        std::cout << "arg #" << i << ":\t[" << argv[i] << "]" << std::endl;
    }

    std::vector<std::string> lines;

    if (argc == 2) {
        lines = read_file(argv[1]);
    }
    else {
        std::string line;
        while (std::getline(std::cin, line)) lines.emplace_back(line);
    }

    for (int i = 0; i < lines.size(); ++i) {
        std::cout << "line #" << i << ":\t[" << lines[i] << "]" << std::endl;
    }
}
