#pragma once

#include <iostream>
#include <set>
#include <sstream>
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

#define BOLD(str) "\033[1m" str "\033[m"
#define RED(str) "\033[31m" str "\033[m"
#define GREEN(str) "\033[32m" str "\033[m"
#define CYAN(str) "\033[36m" str "\033[m"

#define check_true_or_exec(expr, msg, exec, file, line, func, quiet)                       \
    do {                                                                            \
        if (!(expr)) {                                                              \
            if (!(quiet) || DEBUG_CERR) std::cerr << BOLD(RED("error")) ": assertion \"" #expr "\" failed." << std::endl      \
                      << (file) << ": In function `" << (func) << "()`:" << std::endl \
                      << (file) << ":" << (line) << ": " << msg << std::endl;       \
            exec;                                                                   \
        }                                                                           \
    } while (false)

#define check_true_or_ret(expr, msg, ret, file, line, func) check_true_or_exec(expr, msg, return (ret), file, line, func, true)
#define check_true_or_ret_false(expr, msg, file, line, func) check_true_or_ret(expr, msg, false, file, line, func)
#define check_true_or_exit(expr, msg, file, line, func) check_true_or_exec(expr, msg, exit(EXIT_FAILURE), file, line, func, false)

#define check_true_or_ret_false_nomsg(expr) check_true_or_ret_false(expr, "", "", 0, "")
#define check_true_or_ret_false_err(expr, msg, file, line, func) check_true_or_exec(expr, msg, return (false), file, line, func, false)

template <class T>
std::string to_string(const std::set<T>& s) {
    std::stringstream ss;
    ss << "{";
    auto itr = s.begin();
    if (!s.empty()) ss << *itr;
    for (++itr; itr != s.end(); ++itr) ss << ", " << *itr;
    ss << "}";
    return ss.str();
}

template <class T>
std::string to_string(const std::vector<T>& v) {
    std::stringstream ss;
    ss << "[";
    if (v.size() > 0) ss << v[0];
    for (size_t i = 1; i < v.size(); ++i) ss << ", " << v[i];
    ss << "]";
    return ss.str();
}

const std::string SYMBOL_EMPTY = (OnlyAscii ? "{}" : "∅");
const std::string HEADER_CONTEXT = (OnlyAscii ? "Context" : "Γ");
const std::string DEFINITION_SEPARATOR = (OnlyAscii ? "|>" : "▷");
const std::string EMPTY_DEFINIENS = (OnlyAscii ? "#" : "⫫");
const std::string HEADER_ENV = (OnlyAscii ? "Env" : "Δ");
const std::string TURNSTILE = (OnlyAscii ? "|-" : "⊢");
