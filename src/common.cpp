#include "common.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

TextData read_file(const std::string& fname) {
    std::ifstream ifs(fname);
    if (!ifs) {
        std::cerr << "read_file(): " << fname << ": file not found" << std::endl;
        exit(EXIT_FAILURE);
    }
    TextData lines;
    std::string str;
    while (std::getline(ifs, str)) lines.emplace_back(str);
    return lines;
}
