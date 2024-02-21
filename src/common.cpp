#include "common.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

FileError::FileError(const std::string& str) : _msg(str) {}
void FileError::puterror(std::ostream& os) const {
    os << BOLD(RED("FileError")) << ": " << _msg << std::endl;
}

TextData read_lines(const std::string& fname) {
    std::ifstream ifs(fname);
    if (!ifs) throw FileError("read_lines(): " + fname + ": file not found");
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

size_t edit_distance(const std::string_view& a, const std::string_view& b) {
    std::vector<std::vector<size_t>> dp(a.size() + 1, std::vector<size_t>(b.size() + 1, a.size() + b.size()));
    size_t h = a.size(), w = b.size();
    for (size_t i = 0; i <= h; ++i) dp[i][0] = i;
    for (size_t j = 1; j <= w; ++j) dp[0][j] = j;
    for (size_t i = 1; i <= h; ++i) {
        for (size_t j = 1; j <= w; ++j) {
            if (a[i - 1] == b[j - 1]) chmin(dp[i][j], dp[i - 1][j - 1]);
            else chmin(dp[i][j], dp[i - 1][j - 1] + 1);  // substitution
            chmin(dp[i][j], dp[i][j - 1] + 1);           // insertion
            chmin(dp[i][j], dp[i - 1][j] + 1);           // deletion
            if (i >= 2 && j >= 2 &&
                a[i - 1] == b[j - 2] &&
                a[i - 2] == b[j - 1]) chmin(dp[i][j], dp[i - 2][j - 2] + 1);  // swap
        }
    }

    return dp[h][w];
}