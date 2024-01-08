#pragma once

#include <iostream>
#include <string>
#include <vector>

using TextData = std::vector<std::string>;

TextData read_lines(const std::string& fname);
TextData read_lines(std::istream& is);

class FileData : public TextData {
  public:
    FileData(bool from_stdin = false) {
        if (from_stdin) *this = FileData(read_lines(std::cin), "stdin");
    }
    FileData(const std::string& fname) : TextData(read_lines(fname)), _filename(fname) {}
    FileData(const TextData& td, const std::string& srcname): TextData(td), _filename(srcname) {}
    const std::string& name() const { return _filename; }

  private:
    std::string _filename;
};

// ### CONFIG ###
inline constexpr const bool OnlyAscii = false;

template <class T>
void unused(T x) { (void)x; }

template <class T, class... Ts>
void unused(T x, Ts... data) { unused(x), unused(data...); }

const bool DEBUG_CERR = false;
