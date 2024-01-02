#pragma once

#include <string>
#include <vector>

std::vector<std::string> read_file(const std::string& fname);

// ### CONFIG ###
inline constexpr const bool OnlyAscii = false;
