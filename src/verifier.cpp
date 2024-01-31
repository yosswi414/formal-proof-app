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
    // std::cerr << "\t-v                      verbose output for debugging purpose" << std::endl;
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
    // bool is_verbose = false;
    bool is_quiet = false;
    bool skip_check = false;
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
            } else if (arg == "-c") notation = Conventional;
            else if (arg == "-n") notation = New;
            else if (arg == "-r") notation = Rich;
            // else if (arg == "-v") is_verbose = true;
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
    th1.detach();
    try {
        book.read_script(data);
    } catch (InferenceError& e) {
        alive1.store(false);
        e.puterror();

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

    alive1.store(false);
    std::cerr << BOLD(GREEN("verification finished.")) << std::endl;

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
    th2.detach();

    return 0;
}
