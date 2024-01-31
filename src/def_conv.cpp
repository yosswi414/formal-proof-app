#include <iostream>
#include <string>

#include "common.hpp"
#include "environment.hpp"
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
    // std::cerr << "\t-v          verbose output for debugging purpose" << std::endl;
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
    // bool is_verbose = false;
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
            // else if (arg == "-v") is_verbose = true;
            else if (arg == "-h") usage(argv[0], false);
            else if (arg == "-s") is_quiet = true;
            else {
                std::cerr << BOLD(RED("error")) << ": invalid token: " << arg << std::endl;
                usage(argv[0]);
            }
        } else {
            if (fname.size() == 0) fname = arg;
            else {
                std::cerr << BOLD(RED("error")) << ": invalid token: " << arg << std::endl;
                usage(argv[0]);
            }
        }
    }

    if (fname.size() == 0) data = FileData(true);
    else data = FileData(fname);

    std::vector<Token> tokens;
    try {
        tokens = tokenize(data);
    } catch (BaseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }

    Environment env;
    try {
        env = parse_defs(tokens);
    } catch (BaseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }
    if (!is_quiet) {
        switch (notation) {
            case Conventional:
                std::cout << env.repr() << std::endl;
                break;
            case New:
                std::cout << env.repr_new() << std::endl;
                break;
            case Rich:
                std::cout << env.string(false) << std::endl;
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
