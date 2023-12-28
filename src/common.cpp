#include "common.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::vector<std::string> read_file(const std::string& fname) {
    std::ifstream ifs(fname);
    if (!ifs) {
        std::cerr << "read_file(): " << fname << ": file not found" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::vector<std::string> lines;
    std::string str;
    while (std::getline(ifs, str)) lines.emplace_back(str);
    return lines;
}
