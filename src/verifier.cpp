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
    std::cerr << "\t-c          output def file in conventional notation" << std::endl;
    std::cerr << "\t-n          output def file in new notation" << std::endl;
    std::cerr << "\t-r          output def file in rich notation" << std::endl;
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
    std::string fname("");
    int notation = Conventional;
    bool is_verbose = false;
    bool is_quiet = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg[0] == '-') {
            if (arg == "-f") {
                fname = std::string(argv[++i]);
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

    std::stringstream ss;
    for (size_t i = 0; i < data.size(); ++i) {
        ss << data[i];
        int lno;
        std::string op;
        ss >> lno >> op;
        if (lno == -1) break;
        if (!is_quiet && is_verbose) std::cout << "line #" << lno << ": " << op << std::endl;
        if (op == "sort") {
            book.sort();
        } else if (op == "var") {
            int m;
            char x;
            ss >> m >> x;
            book.var(m, x);
        } else if (op == "weak") {
            int m, n;
            char x;
            ss >> m >> n >> x;
            book.weak(m, n, x);
        } else if (op == "form") {
            int m, n;
            ss >> m >> n;
            book.form(m, n);
        } else {
            std::cerr << "read error" << std::endl;
            exit(EXIT_FAILURE);
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
                std::cerr << "invalid notation value = " << notation << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    return 0;
}
