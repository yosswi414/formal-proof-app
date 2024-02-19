// objective: ./test_book3
// generate a book from a given script

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "book.hpp"
#include "common.hpp"
#include "lambda.hpp"

[[noreturn]] void usage(const std::string& execname, bool is_err = true) {
    std::cerr << "usage: " << execname << " [FILE] [OPTION]...\n"
              << std::endl;
    std::cerr << "with no FILE, read stdin. options:\n"
              << std::endl;
    std::cerr << "\t-f FILE     read FILE instead of stdin" << std::endl;
    std::cerr << "\t-c                      output book (judgements) in conventional notation" << std::endl;
    std::cerr << "\t-n                      output book in new notation" << std::endl;
    std::cerr << "\t-r                      output book in rich notation" << std::endl;
    std::cerr << "\t-l LINES                read script until line LINES" << std::endl;
    std::cerr << "\t-d def_file             read def_file for definition reference" << std::endl;
    std::cerr << "\t-o out_file             write output to out_file instead of stdout" << std::endl;
    std::cerr << "\t-e log_file             write error output to log_file instead of stderr" << std::endl;
    std::cerr << "\t--out-def out_def_file  write final environment to out_file" << std::endl;
    std::cerr << "\t--skip-check            skip applicability check of inference rules" << std::endl;
    std::cerr << "\t-v                      verbose output for debugging purpose" << std::endl;
    std::cerr << "\t-i                      run in interactive mode (almost all options are ignored)" << std::endl;
    std::cerr << "\t-s                      suppress output and just verify input (overrides -v)" << std::endl;
    std::cerr << "\t-h                      display this help and exit" << std::endl;
    if (is_err) exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

enum Notation {
    Conventional,
    New,
    Rich
};

std::string strbytes(size_t siz) {
    std::stringstream ss;
    double x = siz;
    if (siz >= 1048576) {
        x /= 1048576;
        ss << (int)x;
        ss << '.' << (int)((x - (int)x) * 10);
        ss << " MB";
        return ss.str();
    }
    if (siz >= 1024) {
        x /= 1024;
        ss << (int)x;
        ss << '.' << (int)((x - (int)x) * 10);
        ss << " KB";
        return ss.str();
    }
    ss << siz << " Bytes";
    return ss.str();
};

int main(int argc, char* argv[]) {
    FileData data;
    std::string fname(""), def_file(""), ofname(""), odefname(""), efname("");
    int notation = Conventional;
    bool is_verbose = false;
    bool is_quiet = false;
    bool skip_check = false;
    bool interactive = false;
    size_t limit = std::string::npos;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg[0] == '-') {
            if (arg == "-f") {
                fname = std::string(argv[++i]);
                continue;
            } else if (arg == "-d") {
                def_file = std::string(argv[++i]);
                continue;
            } else if (arg == "-o") {
                ofname = std::string(argv[++i]);
                continue;
            } else if (arg == "-e") {
                efname = std::string(argv[++i]);
                continue;
            } else if (arg == "--out-def") {
                odefname = std::string(argv[++i]);
                continue;
            } else if (arg == "--skip-check") {
                skip_check = true;
                continue;
            } else if (arg == "-l") {
                limit = std::stoi(argv[++i]);
                continue;
            } else if (arg == "-i") {
                interactive = true;
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

    if (fname.size() > 0) data = FileData(fname);
    else if (!interactive) data = FileData(true);

    Book book(skip_check);
    if (def_file.size() > 0) book.read_def_file(def_file);

    if (limit == std::string::npos) limit = data.size();

    std::stringstream ss;
    auto alive1 = std::atomic_bool(true);
    size_t last_size = 0, ms200 = 0, maxlen = 0;
    auto th1 = std::thread([&alive1, &last_size, &ms200, &maxlen, &book, &data, &limit]() {
        const std::chrono::milliseconds interval(200);
        while (alive1.load()) {
            auto start = std::chrono::system_clock::now();
            std::stringstream ss;
            if (ms200 > 0) {
                std::cerr << "\033[F\033[F" << '\r' << std::flush;
            }
            ss << "[" << (ms200++) / 5 << " secs]";
            ss << "\tprogress: " << book.size() << " / " << limit
               << " (+" << (book.size() - last_size) * 5 << " judgements/sec)";
            std::string text = ss.str();
            ss.clear();
            ss.str("");
            if (text.size() > maxlen) maxlen = text.size();
            else text += std::string(maxlen - text.size(), ' ');
            text += '\n';
            ss << "processing: " << data[std::min(data.size() - 1, book.size())];
            text += ss.str();
            if (ss.str().size() > maxlen) maxlen = ss.str().size();
            else text += std::string(maxlen - ss.str().size(), ' ');
            ss.clear();
            ss.str("");
            std::cerr << text << std::endl;
            last_size = book.size();
            auto end = std::chrono::system_clock::now();
            auto waste = end - start;
            if (waste < interval) std::this_thread::sleep_for(interval - waste);
        }
    });
    if (!is_verbose && data.size() > 0) th1.detach();
    if (data.size() > 0) {
        try {
            book.read_script(data);
        } catch (InferenceError& e) {
            alive1.store(false);
            e.puterror();

            if (!interactive) {
                if (is_verbose) std::cerr << book.repr() << std::endl;

                if (efname.size() > 0) {
                    std::cerr << "extracting the book / env dump... ";
                    std::stringstream err;
                    err << e.str() << std::endl;
                    err << "########## final state of the book ##########" << std::endl;
                    err << book.repr() << std::endl;
                    err << "################ end of book ################" << std::endl;
                    err << "#### the environment in final judgement #####" << std::endl;
                    err << book.back().env()->repr() << std::endl;
                    err << "################ end of book ################" << std::endl;
                    std::cerr << "done." << std::endl;
                    std::string errstr = err.str();
                    std::cerr << "verification has been aborted because of an error." << std::endl;
                    std::cerr << "writing error log to \"" << efname << "\" (" << strbytes(errstr.size()) << ")... ";
                    std::FILE* fp = fopen(efname.c_str(), "wb");
                    std::fwrite(errstr.data(), sizeof(errstr[0]), errstr.size(), fp);
                    std::fclose(fp);
                    std::cerr << "done." << std::endl;
                } else {
                    std::cerr << "run with option \"-e log_file\" for the book / env dump." << std::endl;
                }
                exit(EXIT_FAILURE);
            }
        }
    }

    alive1.store(false);
    std::cerr << BOLD(GREEN("verification finished.")) << std::endl;

    if (interactive) {
        std::cout << "\n"
                  << "[Interactive mode]\n"
                  << "Type \"help\" for available commands.\n\n";
        std::string prompt = ">> ";

        auto read_int = [](int& n, const std::string& arg, int limit = -1) -> bool {
            try {
                n = std::stoi(arg);
            } catch (...) {
                std::cerr << BOLD(RED("error")) ": token \"" << arg << "\" is not a number\n";
                return false;
            }
            if (n < 0) {
                std::cerr << BOLD(RED("error")) ": the number must be nonnegative\n";
                return false;
            }
            if (limit >= 0 && n >= limit) {
                std::cerr << BOLD(RED("error")) ": the number out of range (expected: < " << limit << ")\n";
                return false;
            }
            return true;
        };

        TextData script;
        const Environment env_dummy;
        while (true) {
            const Environment& delta = (book.size() > 0 && book.back().env() ? *book.back().env() : env_dummy);
            std::cout << "[#J: " << book.size() << ", #D: " << delta.size() << "] " << prompt << std::flush;
            std::string line;
            std::getline(std::cin, line);
            std::stringstream ss(line);
            std::vector<std::string> args;
            std::string str;
            while (std::getline(ss, str, ' ')) args.push_back(str);
            if (args[0] == "help") {
                std::cout << "[general command]\n"
                             "help              show this help\n"
                             "exit              end interactive mode and exit\n"
                             "jshow  l [r=l]    show judgements from l-th to r-th\n"
                             "dshow  p          show p-th definition\n"
                             "last   [n=5]      show last n judgements\n"
                             "jsize             show the number of judgements\n"
                             "dsize             show the number of definitions\n"
                             "undo   [n=1]      undo last n inference\n"
                             "load   fname      clear book and read fname\n"
                             "save   fname      save current book to fname\n"
                             "init              clear book\n"
                             "\n"
                             "[available inference command]\n"
                             "sort\n"
                             "var   i x\n"
                             "weak  i j x\n"
                             "form  i j\n"
                             "appl  i j\n"
                             "abst  i j\n"
                             "conv  i j\n"
                             "def   i j name\n"
                             "defpr i j name\n"
                             "inst  i n j1...jn p\n";
            } else if (args[0] == "exit") {
                exit(EXIT_SUCCESS);
            } else if (args[0] == "jshow") {
                do {
                    if (args.size() != 2 && args.size() != 3) {
                        std::cerr << "usage: " << args[0] << " l [r=l]\n";
                        break;
                    }
                    int l, r;
                    if (read_int(l, args[1], book.size())) break;
                    r = l;
                    if (args.size() == 3 && !read_int(r, args[2], book.size())) break;
                    for (int i = l; i <= r; ++i) std::cout << "[" << i << "]: " << book[i].string_simple() << "\n";
                } while (false);

            } else if (args[0] == "dshow") {
                do {
                    if (args.size() != 2) {
                        std::cerr << "usage: " << args[0] << " p\n";
                        break;
                    }
                    int p;
                    if (!read_int(p, args[1], delta.size())) break;
                    std::cout << "{" << p << "}: " << delta[p]->string() << std::endl;
                } while (false);
            } else if (args[0] == "last") {
                do {
                    if (args.size() != 1 && args.size() != 2) {
                        std::cerr << "usage: " << args[0] << " [n=5]\n";
                        break;
                    }
                    int n;
                    n = std::min(5ul, book.size());
                    if (args.size() == 2 && !read_int(n, args[1], book.size())) break;
                    for (size_t i = book.size() - n; i < book.size(); ++i) std::cout << "[" << i << "]: " << book[i].string_simple() << "\n";
                } while (false);
            } else if (args[0] == "jsize") {
                if (args.size() != 1) std::cerr << "usage: " << args[0] << "\n";
                else std::cout << "# of Judgements = " << book.size() << "\n";
            } else if (args[0] == "dsize") {
                if (args.size() != 1) std::cerr << "usage: " << args[0] << "\n";
                else std::cout << "# of Definitions = " << delta.size() << "\n";
            } else if (args[0] == "undo") {
                do {
                    if (args.size() != 1 && args.size() != 2) {
                        std::cerr << "usage: " << args[0] << " [n=1]\n";
                        break;
                    }
                    int n;
                    n = std::min(1ul, book.size());
                    if (args.size() == 2 && !read_int(n, args[1], book.size())) break;
                    book.resize(book.size() - n, Judgement(nullptr, nullptr, nullptr));
                    script.clear();
                } while (false);
            } else if (args[0] == "load") {
            } else if (args[0] == "save") {
            } else if (args[0] == "init") {
                do {
                    if (args.size() != 1) {
                        std::cerr << "usage: " << args[0] << "\n";
                        break;
                    }
                    Book().swap(book);
                } while (false);
            } else if (args[0] == "sort") {
                do {
                    if (args.size() != 1) {
                        std::cerr << "usage: " << args[0] << "\n";
                        break;
                    }
                    try {
                        book.sort();
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "var") {
                do {
                    if (args.size() != 3) {
                        std::cerr << "usage: " << args[0] << " i x\n";
                        break;
                    }
                    int i;
                    if (!read_int(i, args[1], book.size())) break;
                    try {
                        book.var(i, args[2]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    if (is_verbose) {
                        std::cout << book[i].string_simple() << "\n"
                                  << "---------- " << args[2] << "\n";
                    }
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "weak") {
                do {
                    if (args.size() != 4) {
                        std::cerr << "usage: " << args[0] << " i j x\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.weak(i, j, args[3]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    if (is_verbose) {
                        std::cout << book[i].string_simple() << "\n"
                                  << book[j].string_simple() << "\n"
                                  << "---------- " << args[3] << "\n";
                    }
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "form") {
                do {
                    if (args.size() != 3) {
                        std::cerr << "usage: " << args[0] << " i j\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.form(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    if (is_verbose) {
                        std::cout << book[i].string_simple() << "\n"
                                  << book[j].string_simple() << "\n"
                                  << "----------\n";
                    }
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "appl") {
                do {
                    if (args.size() != 3) {
                        std::cerr << "usage: " << args[0] << " i j\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.appl(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    if (is_verbose) {
                        std::cout << book[i].string_simple() << "\n"
                                  << book[j].string_simple() << "\n"
                                  << "----------\n";
                    }
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "abst") {
                do {
                    if (args.size() != 3) {
                        std::cerr << "usage: " << args[0] << " i j\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.abst(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    if (is_verbose) {
                        std::cout << book[i].string_simple() << "\n"
                                  << book[j].string_simple() << "\n"
                                  << "----------\n";
                    }
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "conv") {
                do {
                    if (args.size() != 3) {
                        std::cerr << "usage: " << args[0] << " i j\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.conv(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    if (is_verbose) {
                        std::cout << book[i].string_simple() << "\n"
                                  << book[j].string_simple() << "\n"
                                  << "----------\n";
                    }
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "def") {
                do {
                    if (args.size() != 4) {
                        std::cerr << "usage: " << args[0] << " i j name\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.def(i, j, args[3]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "defpr") {
                do {
                    if (args.size() != 4) {
                        std::cerr << "usage: " << args[0] << " i j name\n";
                        break;
                    }
                    int i, j;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(j, args[2], book.size())) break;
                    try {
                        book.defpr(i, j, args[3]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else if (args[0] == "inst") {
                do {
                    if (args.size() < 4) {
                        std::cerr << "usage: " << args[0] << " i n j1...jn p\n";
                        break;
                    }
                    int i, n, j, p;
                    if (!read_int(i, args[1], book.size())) break;
                    if (!read_int(n, args[2])) break;
                    std::vector<size_t> js;
                    for (int idx = 0; idx < n; ++idx) {
                        if (!read_int(j, args[idx + 3], book.size())) break;
                        js.push_back(j);
                    }
                    if ((int)js.size() != n) break;
                    if (!read_int(p, args[n + 3], delta.size())) break;
                    try {
                        book.inst(i, n, js, p);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    script.push_back(line);
                    std::cout << "res [" << book.size() - 1 << "]: " << book.back().string_simple() << "\n";
                } while (false);
            } else {
                std::cerr << BOLD(RED("error")) << ": unknown command \"" << args[0] << "\"\n"
                                                                                        "Type \"help\" for available commands.\n";
            }
            std::cout << std::endl;
        }
        return 0;
    }

    // std::ofstream ofs(ofname);
    if (!is_quiet || ofname.size() > 0) {
        std::string book_data;
        switch (notation) {
            case Conventional:
                book_data = book.repr();
                break;
            case New:
                book_data = book.repr_new();
                break;
            case Rich:
                book_data = book.string();
                break;
            default:
                check_true_or_exit(
                    false,
                    "invalid notation value = " << notation,
                    __FILE__, __LINE__, __func__);
        }
        std::cerr << "outputting book data (" << strbytes(book_data.size()) << ")... " << std::flush;

        if (ofname.size() > 0) {
            std::FILE* fp = std::fopen(ofname.c_str(), "wb");
            std::fwrite(book_data.data(), sizeof(book_data[0]), book_data.size(), fp);
            std::fclose(fp);
        } else {
            std::cout << book_data;
        }
        std::cerr << "done." << std::endl;
    }

    if (odefname.size() > 0) {
        std::string envrepr = book.back().env()->repr();
        std::cerr << "outputting def_file data (" << strbytes(envrepr.size()) << ")... " << std::flush;
        // std::ofstream odefs(odefname);
        // odefs << envrepr;
        // odefs.close();
        std::FILE* fp = std::fopen(odefname.c_str(), "wb");
        std::fwrite(envrepr.data(), sizeof(envrepr[0]), envrepr.size(), fp);
        std::fclose(fp);
    }

    std::cerr << "done. " << std::endl;
    ms200 = maxlen = 0;
    auto alive2 = std::atomic_bool(true);
    auto th2 = std::thread([&alive2, &ms200, &maxlen]() {
        const std::chrono::milliseconds interval(200);
        while (alive2.load()) {
            auto start = std::chrono::system_clock::now();
            std::stringstream ss;
            if (ms200 > 0) std::cerr << "\033[F" << '\r' << std::flush;
            ss << "[" << (ms200++) / 5 << " secs] waiting for file I/O...";
            std::string text = ss.str();
            ss.clear();
            ss.str("");
            if (text.size() > maxlen) maxlen = text.size();
            else text += std::string(maxlen - text.size(), ' ');
            std::cerr << text << std::endl;
            auto end = std::chrono::system_clock::now();
            auto waste = end - start;
            if (waste < interval) std::this_thread::sleep_for(interval - waste);
        }
    });
    if (!is_verbose) th2.detach();

    return 0;
}
