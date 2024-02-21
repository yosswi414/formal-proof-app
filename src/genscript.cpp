#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <thread>

#include "common.hpp"
#include "environment.hpp"
#include "inference.hpp"
#include "lambda.hpp"
#include "parser.hpp"

[[noreturn]] void usage(const std::string& execname, bool is_err = true) {
    std::cerr << "usage: " << execname << " [FILE] [OPTION]...\n"
              << std::endl;
    std::cerr << "with no FILE, read stdin.\n"
              << "without option -t, script of the last definition of input will be generated.\n"
              << "options:\n"
              << std::endl;
    std::cerr << "\t-f FILE      read FILE instead of stdin" << std::endl;
    std::cerr << "\t-o out_file  output script to out_file instead of stdout" << std::endl;
    std::cerr << "\t-t def       output script only containing def and dependent definitions" << std::endl;
    std::cerr << "\t--dry-run    output dependency of def given with -t and exit" << std::endl;
    std::cerr << "\t-v           verbose output for debugging purpose" << std::endl;
    std::cerr << "\t-s           suppress output and just verify input (overrides -v)" << std::endl;
    std::cerr << "\t-h           display this help and exit" << std::endl;
    if (is_err) exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

extern size_t issued_rules, cache_hit, genscr_called;
auto alive_prog_chk = std::atomic_bool(true);
const size_t time_unit_ms = 97;
std::string current_def;
void progress_check() {
    size_t last_cnt = 0, time_counter = 0, maxlen = 0;
    size_t last_try = 0, last_hit = 0;
    const std::chrono::milliseconds interval(time_unit_ms);
    std::this_thread::sleep_for(interval);
    while (alive_prog_chk.load()) {
        auto start = std::chrono::system_clock::now();
        std::stringstream ss;
        if (time_counter > 0) {
            std::cerr << "\033[F\033[F" << '\r' << std::flush;
        }
        ss << "[" << (++time_counter) * time_unit_ms / 1000 << " secs]";
        ss << " issued rules: " << issued_rules
           << " (+" << (issued_rules - last_cnt) * 1000 / time_unit_ms << " judgements/sec, cache hit: "
           << (last_try == genscr_called ? 0 : (double)(cache_hit - last_hit) / (genscr_called - last_try)) * 100 << " %)";
        std::string text = ss.str();
        if (text.size() > maxlen) maxlen = ss.str().size();
        else text += std::string(maxlen - ss.str().size(), ' ');
        ss.clear();
        ss.str("");

        text += '\n';
        ss << "processing: " << current_def;
        text += ss.str();
        if (ss.str().size() > maxlen) maxlen = ss.str().size();
        else text += std::string(maxlen - ss.str().size(), ' ');
        ss.clear();
        ss.str("");
        std::cerr << text << std::endl;
        last_cnt = issued_rules;
        last_hit = cache_hit;
        last_try = genscr_called;
        auto end = std::chrono::system_clock::now();
        auto waste = end - start;
        if (waste < interval) std::this_thread::sleep_for(interval - waste);
    }
};

void finalize(int code) {
    alive_prog_chk.store(false);
    exit(code);
}

int main(int argc, char* argv[]) {
    FileData data;
    std::string fname(""), target_def_name(""), ofname("");
    bool is_verbose = false;
    bool is_quiet = false;
    bool dry_run = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg[0] == '-') {
            if (arg == "-f") {
                fname = std::string(argv[++i]);
                continue;
            } else if (arg == "-o") {
                ofname = std::string(argv[++i]);
                continue;
            } else if (arg == "-t") {
                target_def_name = std::string(argv[++i]);
                continue;
            } else if (arg == "-v") is_verbose = true;
            else if (arg == "-h") usage(argv[0], false);
            else if (arg == "-s") is_quiet = true;
            else if (arg == "--dry-run") dry_run = true;
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
    if (dry_run && target_def_name.size() == 0) {
        std::cerr << BOLD(RED("error")) << ": target definition (-t) is not given" << std::endl;
        usage(argv[0]);
    }

    if (is_verbose) {
        std::cerr << "Loading definition file... " << std::flush;
    }

    if (fname.size() == 0) data = FileData(true);
    else data = FileData(fname);

    if (is_verbose) {
        std::cerr << BOLD(GREEN("OK")) "\n";
        std::cerr << "Tokenizing file... " << std::flush;
    }

    std::vector<Token> tokens;
    try {
        tokens = tokenize(data);
    } catch (BaseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }

    if (is_verbose) {
        std::cerr << BOLD(GREEN("OK")) "\n";
        std::cerr << "Parsing file... " << std::flush;
    }

    Environment env;
    try {
        env = parse_defs(tokens);
    } catch (BaseError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }

    if (is_verbose) {
        std::cerr << BOLD(GREEN("OK")) "\n";
        std::cerr << "Deducing definitions... " << std::endl;
    }

    std::thread th(progress_check);
    th.detach();

    RulePtr objective;
    if (target_def_name.size() > 0) {
        int idx0 = env.lookup_index(target_def_name);
        if (idx0 < 0) {
            std::cerr << "error: no such constant defined in file: " + target_def_name << std::endl;
            finalize(EXIT_FAILURE);
        }

        using psi = std::pair<std::string, int>;
        std::map<int, psi> resolved;
        std::queue<psi> unresolved;

        unresolved.push({target_def_name, -1});

        while (!unresolved.empty()) {
            psi q = unresolved.front();
            unresolved.pop();
            int idx = env.lookup_index(q.first);
            if (idx < 0) {
                if (q.second < 0) {
                    std::cerr << "error: no such constant defined in file: " + target_def_name << std::endl;
                } else {
                    std::cerr << "error: undefined constant \"" + q.first + "\" found\n";
                    std::cerr << "       during resolving dependency in \"" + env[q.second]->definiendum() + "\"" << std::endl;
                }
                finalize(EXIT_FAILURE);
            }
            resolved[idx] = q;  // always overwrite in BFS to take the earliest dependency
            auto unres_next = extract_constant(env[idx]);
            for (auto&& c : unres_next) unresolved.push({c, idx});
        }

        if (dry_run || (!is_quiet && is_verbose)) {
            for (auto&& [idx, cp] : resolved) {
                const std::string& c = cp.first;
                int par = cp.second;
                std::cout << "(" << idx << ", (" << c << ", " << par << ")) ->" << std::endl;
                std::cout << "idx " << idx << ": " << c << "\n\t:= " << env[idx] << "\n";
                if (par < 0) {
                    std::cout << "\t(root)\n";
                } else {
                    std::cout << "\t(called in " << par << ": " << env[par]->definiendum() << ")\n";
                }
            }
            std::cout << std::flush;
        }

        std::map<int, RulePtr> proofs;
        std::shared_ptr<Environment> delta = std::make_shared<Environment>();
        std::shared_ptr<Context> gamma_dummy = std::make_shared<Context>();
        try {
            // get_script(star, std::make_shared<Environment>(env), gamma_dummy);
            for (auto&& [idx, cp] : resolved) {
                auto def = env[idx];
                delta->push_back(def);
                current_def = def->definiendum();
                proofs[idx] = get_script(star, delta, gamma_dummy);
                objective = proofs[idx];
            }
        } catch (DeductionError& e) {
            e.puterror();
            finalize(EXIT_FAILURE);
        } catch (TypeError& e) {
            e.puterror();
            finalize(EXIT_FAILURE);
        }
    } else {
        std::map<int, RulePtr> proofs;
        std::shared_ptr<Environment> delta = std::make_shared<Environment>();
        std::shared_ptr<Context> gamma_dummy = std::make_shared<Context>();
        try {
            for (size_t idx = 0; idx < env.size(); ++idx) {
                auto def = env[idx];
                delta->push_back(def);
                current_def = def->definiendum();
                proofs[idx] = get_script(star, delta, gamma_dummy);
                objective = proofs[idx];
            }
        } catch (DeductionError& e) {
            e.puterror();
            finalize(EXIT_FAILURE);
        } catch (TypeError& e) {
            e.puterror();
            finalize(EXIT_FAILURE);
        }
    }
    alive_prog_chk.store(false);

    if (is_verbose) {
        std::cerr << "\n" BOLD(GREEN("DEDUCTION COMPLETE")) "\n";
        std::cerr << "Generating script... " << std::flush;
    }

    TextData odata;
    try {
        generate_script(objective, odata);
    } catch (DeductionError& e) {
        e.puterror();
        exit(EXIT_FAILURE);
    }

    if (is_verbose) {
        std::cerr << BOLD(GREEN("OK")) "\n";
        std::cerr << "Writing the script to output file..." << std::flush;
    }

    if (!is_quiet) {
        if (ofname.size() > 0) {
            std::ofstream ofs(ofname);
            if (!ofs) {
                ofs.close();
                std::cerr << "error: could not open file: " << ofname << std::endl;
                exit(EXIT_FAILURE);
            }
            for (auto&& line : odata) ofs << line << "\n";
            ofs << "-1" << std::endl;
            ofs.close();
        } else {
            for (auto&& line : odata) std::cout << line << "\n";
            std::cout << "-1" << std::endl;
        }
    }

    if (is_verbose) {
        std::cerr << BOLD(GREEN("OK")) << std::endl;
    }

    return 0;
}
