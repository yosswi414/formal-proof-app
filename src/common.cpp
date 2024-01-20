#include "common.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

TextData read_lines(const std::string& fname) {
    std::ifstream ifs(fname);
    if (!ifs) {
        std::cerr << BOLD(RED("error")) << ": read_lines(): " << fname << ": file not found" << std::endl;
        exit(EXIT_FAILURE);
    }
    TextData lines;
    std::string str;
    while (std::getline(ifs, str)) lines.emplace_back(str);
    return lines;
}

TextData read_lines(std::istream& is = std::cin) {
    TextData lines;
    std::string str;
    while (std::getline(is, str)) lines.emplace_back(str);
    return lines;
}
