#include <iostream>
#include <string>

#include "common.hpp"
#include "lambda.hpp"
#include "parser.hpp"

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
    std::cerr << "\t-s          suppress output and just verify input" << std::endl;
    std::cerr << "\t-h          display this help and exit" << std::endl;
    if (is_err) exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    FileData data;
    std::string fname("");
    int notation = 0;
    bool is_verbose = false;
    bool is_quiet = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg[0] == '-') {
            if (arg == "-f") {
                fname = std::string(argv[++i]);
                continue;
            } else if (arg == "-c") notation = 0;
            else if (arg == "-n") notation = 1;
            else if (arg == "-r") notation = 2;
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

    auto tokens = tokenize(data);

    if (DEBUG_CERR || is_verbose) {
        for (size_t idx = 0; idx < tokens.size() && idx < 300; ++idx) {
            std::cerr << "token #" << idx << ": " << tokens[idx] << std::endl;
        }
    }

    Environment env;
    try {
        env = parse(tokens);
    } catch (BaseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    if (!is_quiet) {
        switch (notation) {
            case 0:
                std::cout << env.repr() << std::endl;
                break;
            case 1:
                std::cout << env.repr_new() << std::endl;
                break;
            case 2:
                std::cout << env.string(false) << std::endl;
                break;
            default:
                std::cerr << "invalid notation value = " << notation << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    return 0;
}