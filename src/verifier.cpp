// objective: ./test_book3
// generate a book from a given script

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "book.hpp"
#include "common.hpp"
#include "inference.hpp"
#include "lambda.hpp"
#include "parser.hpp"

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
    auto alive1 = std::atomic_bool(false);
    size_t last_size = 0, time_counter = 0, time_unit_ms = 200, maxlen = 0;

    auto progress_check = [&alive1, &last_size, &time_counter, &time_unit_ms, &maxlen, &book, &data, &limit]() {
        const std::chrono::milliseconds interval(time_unit_ms);
        std::this_thread::sleep_for(interval);
        while (alive1.load()) {
            auto start = std::chrono::system_clock::now();
            std::stringstream ss;
            if (time_counter > 0) {
                std::cerr << "\033[F\033[F" << '\r' << std::flush;
            }
            ss << "[" << (++time_counter) * time_unit_ms / 1000 << " secs]";
            ss << "\tprogress: " << book.size() << " / " << limit
               << " (+" << (book.size() - last_size) * 5 << " judgements/sec)";
            std::string text = ss.str();
            ss.clear();
            ss.str("");
            if (text.size() > maxlen) maxlen = text.size();
            else text += std::string(maxlen - text.size(), ' ');
            text += '\n';
            ss << "processing: ";
            if (data.size() > 0) ss << data[std::min(data.size() - 1, book.size())];
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
    };

    if (data.size() > 0) {
        auto th1 = std::thread(progress_check);
        bool is_success = true;
        if (!is_verbose) alive1.store(true);
        th1.detach();
        try {
            book.read_script(data);
        } catch (InferenceError& e) {
            alive1.store(false);
            is_success = false;
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

        if (is_success) std::cerr << BOLD(GREEN("verification finished.")) << std::endl;
        alive1.store(false);
    }

    if (interactive) {
        const std::string help_reminder = "Type \"help\" for available commands.";
        const std::string prompt = "$";
        const std::vector<std::vector<std::string>> cmd_list = {
            {"[general command]"},
            {"help", "", "show this help"},
            {"exit", "", "end interactive mode and exit"},
            {"quit", "", "alias of exit"},
            {"jshow", "l [r=l]", "show judgements from l-th to r-th"},
            {"dshow", "l [r=l]", "show definitions from l-th to r-th"},
            {"dnlookup", "names...", "show definitions of names"},
            {"tail", "[n=5]", "show last n judgements"},
            {"jsize", "", "show the number of judgements"},
            {"dsize", "", "show the number of definitions"},
            {"undo", "[n=1]", "undo last n inference"},
            {"load", "fname", "clear book and read fname"},
            {"save", "fname", "save current book to fname"},
            {"init", "", "clear book"},
            {"clear", "", "alias of init"},
            {"jump", "n", "refer to n-th judgement as the current context (resets with n = -1)"},
            {"type", "formula", "find the type of given formula in current context"},
            {"[derivation command]"},
            {"sort", ""},
            {"var", "i var_name"},
            {"weak", "i j var_name"},
            {"form", "i j"},
            {"appl", "i j"},
            {"abst", "i j"},
            {"conv", "i j"},
            {"def", "i j def_name"},
            {"defpr", "i j def_name"},
            {"inst", "i n j1...jn p"},
        };
        std::array<size_t, 2> width_cmd = {0, 0}, width_fmt = {0, 0};
        {
            size_t cnt_title = 0;
            for (auto&& cmd : cmd_list) {
                if (cmd.size() <= 1) {
                    ++cnt_title;
                    continue;
                }
                width_cmd[cnt_title - 1] = std::max(width_cmd[cnt_title - 1], cmd[0].size());
                width_fmt[cnt_title - 1] = std::max(width_fmt[cnt_title - 1], cmd[1].size());
            }
            width_cmd[0] += 3;
            width_fmt[0] += 3;
            width_cmd[1] += 3;
        }

        std::string _help_description =
            "[prompt]\n"
            "#J denotes the number of judgements on the book.\n"
            "#D denotes the number of defined definitions (by def or defpr).\n"
            "@n represents that the n-th judgement is refered to as the current context\n"
            "(if ommited, the last judgement is referred to)\n"
            "\n";
        {
            std::stringstream ss;
            size_t cnt_title = 0;
            for (auto&& cmd : cmd_list) {
                if (cmd.size() >= 2) {
                    if (cnt_title == 1) {
                        ss << std::setw(width_cmd[0]) << std::left << cmd[0];
                        ss << std::setw(width_fmt[0]) << std::left << cmd[1];
                        ss << cmd[2];
                    } else {
                        ss << std::setw(width_cmd[1]) << std::left << cmd[0];
                        ss << std::setw(width_fmt[1]) << std::left << cmd[1];
                    }

                } else {
                    ++cnt_title;
                    ss << "\n"
                       << cmd[0] << "\n";
                    if (cnt_title == 1) {
                        ss << std::setw(width_cmd[0]) << std::left << "name";
                        ss << std::setw(width_fmt[0]) << std::left << "args";
                        ss << "description\n";
                        ss << std::string(width_cmd[0] + width_fmt[0] + sizeof("description"), '-');
                    } else {
                        ss << std::setw(width_cmd[1]) << std::left << "name";
                        ss << std::setw(width_fmt[1]) << std::left << "args";
                        ss << "\n";
                        ss << std::string(width_cmd[1] + width_fmt[1], '-');
                    }
                }

                ss << "\n";
            }
            _help_description += ss.str();
        }

        const std::string help_description = _help_description;

        auto read_int = [](int& n, const std::string& arg) -> bool {
            try {
                n = std::stoi(arg);
            } catch (...) {
                std::cerr << BOLD(RED("error")) ": token \"" << arg << "\" is not a number\n";
                return false;
            }
            return true;
        };
        auto read_index = [&read_int](int& n, const std::string& arg, int limit = -1, bool asis = false) -> bool {
            if (limit == 0) {
                std::cerr << BOLD(RED("error")) ": the list is empty\n";
                return false;
            }
            if (!read_int(n, arg)) return false;

            if (limit > 0 && (n < -limit || limit <= n)) {
                std::cerr << BOLD(RED("error")) ": the number out of range (expected: " << -limit << " <= n < " << limit << ")\n";
                return false;
            }
            if (!asis && limit > 0) n = (n + limit) % limit;
            return true;
        };
        auto read_nonneg = [&read_int](int& n, const std::string& arg, int limit = -1) -> bool {
            if (limit == 0) {
                std::cerr << BOLD(RED("error")) ": the list is empty\n";
                return false;
            }
            if (!read_int(n, arg)) return false;
            if (n < 0) {
                std::cerr << BOLD(RED("error")) ": the number must be nonnegative\n";
                return false;
            }

            if (limit >= 0 && n >= limit) {
                std::cerr << BOLD(RED("error")) ": the number out of range (expected: 0 <= n < " << limit << ")\n";
                return false;
            }
            return true;
        };
        std::function<void(size_t)> print_judge = [&book](size_t idx) -> void {
            std::cout << "[" << idx << "]: " << book[idx].string_simple() << "\n";
        };
        auto check_argc = [&is_verbose, &read_nonneg](const std::vector<std::string>& args, const std::string& format) -> bool {
            int argc = args.size();

            // variable length arguments
            if (args[0] == "inst") {
                int n;
                if (argc < 4 || !read_nonneg(n, args[2]) || argc != n + 4) {
                    std::cerr << "usage: " << args[0] << " " << format << "\n";
                    return false;
                }
                return true;
            }
            if (args[0] == "type") {
                if (argc < 2) {
                    std::cerr << "usage: " << args[0] << " " << format << "\n";
                    return false;
                }
                return true;
            }
            if (args[0] == "dnlookup") {
                if (argc < 2) {
                    std::cerr << "usage: " << args[0] << " " << format << "\n";
                    return false;
                }
                return true;
            }

            std::stringstream ss;
            ss << format;
            std::string arg_format;
            int min_c = 1, opts = 0;
            while (ss >> arg_format) {
                if (arg_format[0] == '[') ++opts;
                else if (opts > 0) {
                    std::cerr << BOLD(RED("internal error"))
                              << ": ill-formed command format: \"" << format << "\"\n"
                              << "all optional parameters must come after required parameters" << std::endl;
                    exit(EXIT_FAILURE);
                } else ++min_c;
            }
            int max_c = min_c + opts;
            if (is_verbose) {
                std::cerr << "check_argc: " << min_c << " <= " << argc << " <= " << max_c << std::endl;
            }

            if (argc < min_c || max_c < argc) {
                std::cerr << BOLD(RED("error")) ": given arguments don't match the format\n";
                std::cerr << "usage: " << args[0] << " " << format << "\n";
                return false;
            }
            return true;
        };

        const Environment env_dummy;
        const Judgement judge_dummy{nullptr, nullptr, nullptr};

        std::cout << "[Interactive mode]\n"
                  << help_reminder << "\n\n";

        TextData script;

        int current_line = -1;

        while (true) {
            const Environment& delta = (book.size() > 0 && book.back().env() ? *book.back().env() : env_dummy);
            std::function<void(size_t)> print_def = [&delta](size_t idx) -> void {
                std::cout << "{" << idx << "}: " << delta[idx]->string() << "\n";
            };

            std::cout << "[#J: " << book.size() << ", #D: " << delta.size() << "] ";
            if (current_line != -1) std::cout << "@" << current_line << " ";
            std::cout << prompt << " " << std::flush;

            std::string line;
            std::getline(std::cin, line);
            std::stringstream ss;
            ss << line;
            std::vector<std::string> args;
            std::string str;
            while (ss >> str) args.push_back(str);
            if (args.size() == 0) continue;

            // print arg list
            if (is_verbose) {
                std::cerr << "args: [" << args[0];
                for (size_t i = 1; i < args.size(); ++i) std::cerr << ", " << args[i];
                std::cerr << "]" << std::endl;
            }

            bool inf_success = false;

            bool cmd_found = false, fmt_well_formed = false;
            size_t closest_dist = 1000, closest_len = 0;
            std::string closest_cmd;
            for (auto&& cmd : cmd_list) {
                if (cmd.size() <= 1) continue;
                if (cmd[0] == args[0]) {
                    cmd_found = true;
                    fmt_well_formed = check_argc(args, cmd[1]);
                    break;
                }
                size_t dist = edit_distance(args[0], cmd[0]);
                if (dist < closest_dist || (dist == closest_dist && closest_len < cmd[0].size())) {
                    closest_dist = dist;
                    closest_cmd = cmd[0];
                    closest_len = cmd[0].size();
                }
            }
            if (is_verbose) std::cerr << "closest command: " << closest_cmd << " (dist = " << closest_dist << ")\n";
            if (!cmd_found) {
                std::cerr << BOLD(RED("error"))
                          << ": Unknown command: \"" << args[0] << "\".";
                if (closest_dist <= 1) {
                    std::cerr << " Did you mean \"" << closest_cmd << "\"?";
                }
                std::cerr << "\n"
                          << help_reminder << "\n";
            } else if (!fmt_well_formed) {  // nop
            } else if (args[0] == "help") {
                std::cout << help_description;
            } else if (args[0] == "exit" || args[0] == "quit") {
                break;  // exit while(true) loop
            } else if (args[0] == "jshow" || args[0] == "dshow") {
                do {
                    int l, r;
                    bool is_j = (args[0] == "jshow");
                    size_t limit = is_j ? book.size() : delta.size();
                    if (!read_nonneg(l, args[1], limit)) break;
                    r = l;
                    if (args.size() == 3 && !read_nonneg(r, args[2], limit)) break;
                    auto show = is_j ? print_judge : print_def;
                    for (int i = l; i <= r; ++i) show(i);
                } while (false);
            } else if (args[0] == "dnlookup") {
                for (size_t i = 1; i < args.size(); ++i) {
                    int idx = delta.lookup_index(args[i]);
                    if (idx < 0) {
                        std::cerr << BOLD(RED("error")) ": Name \"" << args[i] << "\" undefined\n";
                    } else {
                        std::cout << args[i] << " -> ";
                        print_def(idx);
                    }
                }
            } else if (args[0] == "tail") {
                do {
                    int n;
                    n = std::min(5ul, book.size());
                    if (args.size() == 2 && !read_index(n, args[1], book.size())) break;
                    if (n < 0) {
                        for (int i = 0; i < -n; ++i) std::cout << "[" << i << "]: " << book[i].string_simple() << "\n";
                    }
                    for (size_t i = book.size() - n; i < book.size(); ++i) std::cout << "[" << i << "]: " << book[i].string_simple() << "\n";
                } while (false);
            } else if (args[0] == "jsize") {
                std::cout << "# of Judgements = " << book.size() << "\n";
            } else if (args[0] == "dsize") {
                std::cout << "# of Definitions = " << delta.size() << "\n";
            } else if (args[0] == "undo") {
                do {
                    int n;
                    n = std::min(1ul, book.size());
                    if (args.size() == 2 && !read_index(n, args[1], book.size())) break;
                    book.resize(book.size() - n, judge_dummy);
                    script.clear();
                } while (false);
            } else if (args[0] == "load") {
                do {
                    if (book.size() > 0) {
                        std::cerr << BOLD(RED("error")) << ": the book is not empty\n";
                        std::cerr << "Run \"init\" and discard the current book to load a new script\n";
                        break;
                    }
                    const std::string& ifname = args[1];
                    std::cout << "Reading script from " << ifname << "..." << std::endl;
                    auto th = std::thread(progress_check);
                    alive1.store(true);
                    th.detach();
                    try {
                        data = FileData(ifname);
                        book.read_script(data);
                    } catch (FileError& e) {
                        alive1.store(false);
                        e.puterror();
                        break;
                    } catch (InferenceError& e) {
                        alive1.store(false);
                        e.puterror();
                        std::cout << RED("Warning") << ": The script has been loaded up to the line just before the occurence of the InferenceError.\n";
                        break;
                    }
                    alive1.store(false);
                    std::cout << BOLD(GREEN("OK")) << ": The script has been loaded successfully.\n";

                } while (false);
            } else if (args[0] == "save") {
                const std::string& ofname = args[1];
                if (book.size() == 0) {
                    std::cerr << "The book is empty.\n";
                } else {
                    std::string book_data;
                    std::cout << "Converting book to text... " << std::flush;
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
                    std::cout << BOLD(GREEN("OK")) << "\n";
                    std::cout << "Writing data (" << strbytes(book_data.size()) << ") to " << ofname << "... " << std::flush;

                    std::FILE* fp = std::fopen(ofname.c_str(), "wb");
                    std::fwrite(book_data.data(), sizeof(book_data[0]), book_data.size(), fp);
                    std::fclose(fp);

                    std::cout << BOLD(GREEN("OK")) << "\n";
                }
            } else if (args[0] == "init" || args[0] == "clear") {
                Book().swap(book);
            } else if (args[0] == "jump") {
                do {
                    if (!read_index(current_line, args[1], book.size(), true)) break;
                    if (current_line < -1) current_line += book.size();
                } while (false);
            } else if (args[0] == "type") {
                do {
                    std::stringstream ss;
                    ss << line;
                    std::string expr;
                    ss >> expr;
                    expr = ss.str();
                    const Judgement& judge = (current_line == -1 ? book.back() : book[(current_line + book.size()) % book.size()]);
                    std::shared_ptr<Term> term, type;
                    try {
                        term = parse_lambda(expr, *judge.env());
                        type = get_type(term, judge.env(), judge.context());
                    } catch (ParseError& e) {
                        e.puterror();
                        break;
                    } catch (TypeError& e) {
                        e.puterror();
                        break;
                    }
                    std::cout << judge.env()->string_brief(true, 0) << " ; " << judge.context() << " " << TURNSTILE << " " << term << " : ";
                    if (type) std::cout << type;
                    else std::cout << "(untypable)";
                    std::cout << "\n";
                } while (false);
            } else if (args[0] == "sort") {
                do {
                    try {
                        book.sort();
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }

                    inf_success = true;
                } while (false);
            } else if (args[0] == "var") {
                do {
                    int i;
                    if (!read_index(i, args[1], book.size())) break;
                    try {
                        book.var(i, args[2]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    if (is_verbose) {
                        print_judge(i);
                        std::cout << "---------- " << args[2] << "\n";
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "weak") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.weak(i, j, args[3]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    if (is_verbose) {
                        print_judge(i);
                        print_judge(j);
                        std::cout << "---------- " << args[3] << "\n";
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "form") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.form(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }

                    if (is_verbose) {
                        print_judge(i);
                        print_judge(j);
                        std::cout << "----------\n";
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "appl") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.appl(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    if (is_verbose) {
                        print_judge(i);
                        print_judge(j);
                        std::cout << "----------\n";
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "abst") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.abst(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }

                    if (is_verbose) {
                        print_judge(i);
                        print_judge(j);
                        std::cout << "----------\n";
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "conv") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.conv(i, j);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }

                    if (is_verbose) {
                        print_judge(i);
                        print_judge(j);
                        std::cout << "----------\n";
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "def") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.def(i, j, args[3]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "defpr") {
                do {
                    int i, j;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_index(j, args[2], book.size())) break;
                    try {
                        book.defpr(i, j, args[3]);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    inf_success = true;
                } while (false);
            } else if (args[0] == "inst") {
                do {
                    int i, n, j, p;
                    if (!read_index(i, args[1], book.size())) break;
                    if (!read_nonneg(n, args[2])) break;
                    std::vector<size_t> js;
                    for (int idx = 0; idx < n; ++idx) {
                        if (!read_index(j, args[idx + 3], book.size())) break;
                        js.push_back(j);
                    }
                    if (!read_index(p, args[n + 3], delta.size())) break;
                    try {
                        book.inst(i, n, js, p);
                    } catch (InferenceError& e) {
                        e.puterror();
                        break;
                    }
                    inf_success = true;
                } while (false);
            } else {
                std::cerr << BOLD(RED("internal error"))
                          << ": command \"" << args[0] << "\" on cmd_list not implemented\n";
            }
            if (inf_success) {
                script.push_back(line);
                std::cout << "res ";
                print_judge(book.size() - 1);
            }
            std::cerr << std::flush;
            std::cout << std::endl;
        }
    }

    // std::ofstream ofs(ofname);
    if (book.size() > 0 && !interactive) {
        if ((!is_quiet && is_verbose) || ofname.size() > 0) {
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

            if (ofname.size() > 0) {
                std::cerr << "outputting book data (" << strbytes(book_data.size()) << ")... " << std::flush;
                std::FILE* fp = std::fopen(ofname.c_str(), "wb");
                std::fwrite(book_data.data(), sizeof(book_data[0]), book_data.size(), fp);
                std::fclose(fp);
                std::cerr << "done." << std::endl;
            } else if (is_verbose) {
                std::cout << "\n"
                          << book_data << std::endl;
            }
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
            std::cerr << "done. " << std::endl;
        }
    }

    time_counter = maxlen = 0;
    if (!interactive) {
        auto alive2 = std::atomic_bool(true);
        auto progress_check_io = [&alive2, &time_counter, &time_unit_ms, &maxlen]() {
            const std::chrono::milliseconds interval(time_unit_ms);
            std::this_thread::sleep_for(interval);
            while (alive2.load()) {
                auto start = std::chrono::system_clock::now();
                std::stringstream ss;
                if (time_counter > 0) std::cerr << "\033[F" << '\r' << std::flush;
                ss << "[" << (++time_counter) * time_unit_ms / 1000 << " secs] waiting for file I/O...";
                std::string text = ss.str();
                if (text.size() > maxlen) maxlen = text.size();
                else text += std::string(maxlen - text.size(), ' ');
                std::cerr << text << std::endl;
                ss.clear();
                ss.str("");
                auto end = std::chrono::system_clock::now();
                auto waste = end - start;
                if (waste < interval) std::this_thread::sleep_for(interval - waste);
            }
        };
        auto th2 = std::thread(progress_check_io);
        th2.detach();
    }

    return 0;
}
