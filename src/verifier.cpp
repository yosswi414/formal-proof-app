// objective: ./test_book3
// generate a book from a given script

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <cstdio>

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
    std::cerr << "\t--out-def out_def_file  write final environment to out_file" << std::endl;
    std::cerr << "\t--skip-check            skip applicability check of inference rules" << std::endl;
    std::cerr << "\t-v                      verbose output for debugging purpose" << std::endl;
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

int main(int argc, char* argv[]) {
    FileData data;
    std::string fname(""), def_file(""), ofname(""), odefname("");
    int notation = Conventional;
    bool is_verbose = false;
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

    Book book(skip_check);
    if (def_file.size() > 0) book.read_def_file(def_file);

    if (limit == std::string::npos) limit = data.size();

    std::stringstream ss;
    auto alive1 = std::atomic_bool(true);
    size_t last_size = 0, sec = 0, maxlen = 0;
    auto th1 = std::thread([&alive1, &last_size, &sec, &maxlen, &book, &limit]() {
        const std::chrono::milliseconds interval(1000);
        while (alive1.load()) {
            auto start = std::chrono::system_clock::now();
            std::stringstream ss;
            if (sec > 0) std::cerr << "\033[F" << '\r' << std::flush;
            ss << "[" << sec++ << " secs]";
            ss << "\tprogress: " << book.size() << " / " << limit
               << " (+" << book.size() - last_size << " judgements/sec)";
            std::string text = ss.str();
            ss.clear();
            ss.str("");
            if (text.size() > maxlen) maxlen = text.size();
            else text += std::string(maxlen - text.size(), ' ');
            std::cerr << text << std::endl;
            last_size = book.size();
            auto end = std::chrono::system_clock::now();
            auto waste = end - start;
            if (waste < interval) std::this_thread::sleep_for(interval - waste);
        }
    });
    th1.detach();
    size_t max_diff = 0;
#define chmax(a, b) ((a) < (b) ? ((a) = (b)) : (a))
    for (size_t i = 0; i < limit; ++i) {
        ss << data[i];
        int lno;
        std::string op;
        ss >> lno;
        if (lno == -1) break;
        ss >> op;
        if (!is_quiet && is_verbose) std::cout << "line #" << lno << ": " << op << std::endl;
        if (op == "sort") {
            book.sort();
        } else if (op == "var") {
            size_t idx;
            char x;
            check_true_or_exit(
                ss >> idx >> x,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx);
            book.var(idx, x);
        } else if (op == "weak") {
            size_t idx1, idx2;
            char x;
            check_true_or_exit(
                ss >> idx1 >> idx2 >> x,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.weak(idx1, idx2, x);
        } else if (op == "form") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.form(idx1, idx2);
        } else if (op == "appl") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.appl(idx1, idx2);
        } else if (op == "abst") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.abst(idx1, idx2);
        } else if (op == "conv") {
            size_t idx1, idx2;
            check_true_or_exit(
                ss >> idx1 >> idx2,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.conv(idx1, idx2);
        } else if (op == "def") {
            size_t idx1, idx2;
            std::string a;
            check_true_or_exit(
                ss >> idx1 >> idx2 >> a,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.def(idx1, idx2, a);
        } else if (op == "defpr") {
            size_t idx1, idx2;
            std::string a;
            check_true_or_exit(
                ss >> idx1 >> idx2 >> a,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx1);
            chmax(max_diff, lno - idx2);
            book.defpr(idx1, idx2, a);
        } else if (op == "inst") {
            size_t idx0, n, p;
            check_true_or_exit(
                ss >> idx0 >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx0);
            std::vector<size_t> idxs(n);
            for (auto&& ki : idxs) {
                check_true_or_exit(
                    ss >> ki,
                    op
                        << ": too few arguments are given, or type of argument is wrong"
                        << " (line " << i + 1 << ")",
                    __FILE__, __LINE__, __func__);
                chmax(max_diff, lno - ki);
            }
            check_true_or_exit(
                ss >> p,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            book.inst(idx0, n, idxs, p);
        } else if (op == "cp") {
            size_t idx;
            check_true_or_exit(
                ss >> idx,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx);
            book.cp(idx);
        } else if (op == "sp") {
            size_t idx, n;
            check_true_or_exit(
                ss >> idx >> n,
                op
                    << ": too few arguments are given, or type of argument is wrong"
                    << " (line " << i + 1 << ")",
                __FILE__, __LINE__, __func__);
            chmax(max_diff, lno - idx);
            book.sp(idx, n);
        } else {
            check_true_or_exit(
                false,
                "read error (token: " << op << ")",
                __FILE__, __LINE__, __func__);
        }
        if (!is_quiet && is_verbose) {
            std::cerr << book.string() << std::endl;
        }
        ss.clear();
        ss.str("");
    }

    alive1.store(false);
    std::cerr << "verification finished." << std::endl;
    std::cerr << "max_diff = " << max_diff << std::endl;

    auto strbytes = [](size_t siz) {
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
        std::cerr << "outputting book data (" << strbytes(book_data.size()) << ")... ";
        // if (ofs) {
        //     ofs << book_data;
        //     ofs.close();
        // } 
        if (ofname.size() > 0){
            std::FILE* fp = std::fopen(ofname.c_str(), "wb");
            std::fwrite(book_data.data(), sizeof(book_data[0]), book_data.size(), fp);
            std::fclose(fp);
        } else {
            std::cout << book_data;
        }
        std::cerr << "done." << std::endl;
    }

    if (odefname.size() > 0) {
        std::string envrepr = book.back().env().repr();
        std::cerr << "outputting def_file data (" << strbytes(envrepr.size()) << ")... ";
        // std::ofstream odefs(odefname);
        // odefs << envrepr;
        // odefs.close();
        std::FILE* fp = std::fopen(odefname.c_str(), "wb");
        std::fwrite(envrepr.data(), sizeof(envrepr[0]), envrepr.size(), fp);
        std::fclose(fp);
    }

    std::cerr << "done. " << std::endl;
    sec = maxlen = 0;
    auto alive2 = std::atomic_bool(true);
    auto th2 = std::thread([&alive2, &sec, &maxlen]() {
        const std::chrono::milliseconds interval(1000);
        while (alive2.load()) {
            auto start = std::chrono::system_clock::now();
            std::stringstream ss;
            if (sec > 0) std::cerr << "\033[F" << '\r' << std::flush;
            ss << "[" << sec++ << " secs] waiting for file I/O...";
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
