// objective: ./test_book3
// generate a book from a given script

#include <iostream>
#include <sstream>

#include "common.hpp"
#include "lambda.hpp"

[[noreturn]] void usage(const std::string& execname, bool is_err = true) {
    std::cerr << "usage: " << execname << " [FILE] [OPTION]...\n"
              << std::endl;
    std::cerr << "with no FILE, read stdin. options:\n"
              << std::endl;
    std::cerr << "\t-f FILE     read FILE instead of stdin" << std::endl;
    std::cerr << "\t-c          output book (judgements) in conventional notation" << std::endl;
    std::cerr << "\t-n          output book in new notation" << std::endl;
    std::cerr << "\t-r          output book in rich notation" << std::endl;
    std::cerr << "\t-l LINES    read script until line LINES" << std::endl;
    std::cerr << "\t-d def_file read def_file for definition reference" << std::endl;
    std::cerr << "\t-v          verbose output for debugging purpose" << std::endl;
    std::cerr << "\t-s          suppress output and just verify input (overrides -v)" << std::endl;
    std::cerr << "\t-h          display this help and exit" << std::endl;
    if (is_err) exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

enum Notation {
    Conventional,
    New,
    Rich
};

int main(int argc, char* argv[]) {
    FileData data;
    std::string fname(""), def_file("");
    int notation = Conventional;
    bool is_verbose = false;
    bool is_quiet = false;
    size_t limit = std::string::npos;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg[0] == '-') {
            if (arg == "-f") {
                fname = std::string(argv[++i]);
                continue;
            }           else if (arg == "-d") {
                def_file = std::string(argv[++i]);
                continue;
            } else if (arg == "-l") {
                limit = std::stoi(argv[++i]);
                continue;
            } else if (arg == "-c") notation = Conventional;
            else if (arg == "-n") notation = New;
            else if (arg == "-r") notation = Rich;
            else if (arg == "-v") is_verbose = true;
            else if (arg == "-h") usage(argv[0], false);
            else if (arg == "-s") is_quiet = true;
            else {
                std::cerr << "invalid token: " << arg << std::endl;
                usage(argv[0]);
            }
        } else {
            if (fname.size() == 0) fname = arg;
            else {
                std::cerr << "invalid token: " << arg << std::endl;
                usage(argv[0]);
            }
        }
    }

    if (fname.size() == 0) data = FileData(true);
    else data = FileData(fname);

    Book book;
    if (def_file.size() > 0) book.read_def_file(def_file);

    if (limit == std::string::npos) limit = data.size();

    std::stringstream ss;
    for (size_t i = 0; i < limit; ++i) {
        ss << data[i];
        int lno;
        std::string op;
        ss >> lno >> op;
        if (lno == -1) break;
        if (!is_quiet && is_verbose) std::cout << "line #" << lno << ": " << op << std::endl;
        if (op == "sort") {
            book.sort();
        } else if (op == "var") {
            size_t m;
            char x;
            check_true_or_exit(
                ss >> m >> x,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.var(m, x);
        } else if (op == "weak") {
            size_t m, n;
            char x;
            check_true_or_exit(
                ss >> m >> n >> x,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.weak(m, n, x);
        } else if (op == "form") {
            size_t m, n;
            check_true_or_exit(
                ss >> m >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.form(m, n);
        } else if (op == "appl") {
            size_t m, n;
            check_true_or_exit(
                ss >> m >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.appl(m, n);
        } else if (op == "abst") {
            size_t m, n;
            check_true_or_exit(
                ss >> m >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.abst(m, n);
        } else if (op == "conv") {
            size_t m, n;
            check_true_or_exit(
                ss >> m >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.conv(m, n);
        } else if (op == "def") {
            size_t m, n;
            std::string a;
            check_true_or_exit(
                ss >> m >> n >> a,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.def(m, n, a);
        } else if (op == "defpr") {
            size_t m, n;
            std::string a;
            check_true_or_exit(
                ss >> m >> n >> a,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.defpr(m, n, a);
        } else if (op == "inst") {
            size_t m, n, p;
            check_true_or_exit(
                ss >> m >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            std::vector<size_t> k(n);
            for (auto&& ki : k) {
                check_true_or_exit(
                    ss >> ki,
                    op
                        << ": too few arguments are given, or type of argument is wrong"
                        << " (line " << i + 1 << ")",
                    __FILE__, __LINE__, __func__);
            }
            check_true_or_exit(
                ss >> p,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.inst(m, n, k, p);
        } else if (op == "cp") {
            size_t m;
            check_true_or_exit(
                ss >> m,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.cp(m);
        } else if (op == "sp") {
            size_t m, n;
            check_true_or_exit(
                ss >> m >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.sp(m, n);
        } else {
            check_true_or_exit(
                false,
                "read error (token: " << op << ")",
                __FILE__, __LINE__, __func__);
        }
        if (!is_quiet && is_verbose) std::cout << book.string() << std::endl;
        ss.clear();
        ss.str("");
    }

    if (!is_quiet) {
        switch (notation) {
            case Conventional:
                std::cout << book.repr();
                break;
            case New:
                std::cout << book.repr_new();
                break;
            case Rich:
                std::cout << book.string() << std::endl;
                break;
            default:
                check_true_or_exit(
                    false,
                    "invalid notation value = " << notation,
                    __FILE__, __LINE__, __func__);
        }
    }

    return 0;
}
