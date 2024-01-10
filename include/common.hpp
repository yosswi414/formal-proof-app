#pragma once

#include <iostream>
#include <set>
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
    FileData(const TextData& td, const std::string& srcname) : TextData(td), _filename(srcname) {}
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

template <class T>
std::set<T>& set_union(const std::set<T>& a, const std::set<T>& b, std::set<T>& res) {
    res = a;
    for (auto&& i : b) res.insert(i);
    return res;
}

template <class T>
std::set<T> set_union(const std::set<T>& a, const std::set<T>& b) {
    std::set<T> res(a);
    for (auto&& i : b) res.insert(i);
    return res;
}

template <class T>
std::set<T>& set_union_inplace(std::set<T>& a, const std::set<T>& b) {
    for (auto&& i : b) a.insert(i);
    return a;
}

template <class T>
std::set<T>& set_minus(const std::set<T>& a, const std::set<T>& b, std::set<T>& res) {
    res = a;
    for (auto&& i : b) res.erase(i);
    return res;
}

template <class T>
std::set<T> set_minus(const std::set<T>& a, const std::set<T>& b) {
    std::set<T> res(a);
    for (auto&& i : b) res.erase(i);
    return res;
}

template <class T>
std::set<T>& set_minus_inplace(std::set<T>& a, const std::set<T>& b) {
    for (auto&& i : b) a.erase(i);
    return a;
}

#define check_true(expr)           \
    do {                           \
        if (!(expr)) return false; \
    } while (false);
#define check_false(expr)       \
    do {                        \
        if (expr) return false; \
    } while (false);

