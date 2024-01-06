#pragma once

#include <string>
#include <vector>

using TextData = std::vector<std::string>;

TextData read_file(const std::string& fname);

class FileData : public TextData {
  public:
    FileData(const std::string& fname) : TextData(read_file(fname)), _filename(fname) {}
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
