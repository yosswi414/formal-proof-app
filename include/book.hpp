#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "definition.hpp"
#include "environment.hpp"
#include "judgement.hpp"

class Book : public std::vector<Judgement> {
  public:
    Book(bool skip_check = false);
    Book(const std::vector<Judgement>& list);
    Book(const std::string& scriptname, size_t limit = -1);
    Book(const FileData& fdata, size_t limit = -1);

    // inference rules
    void sort();
    void var(size_t m, char x);
    void weak(size_t m, size_t n, char x);
    void form(size_t m, size_t n);
    void appl(size_t m, size_t n);
    void abst(size_t m, size_t n);
    void conv(size_t m, size_t n);
    void def(size_t m, size_t n, const std::string& a);
    void defpr(size_t m, size_t n, const std::string& a);
    void inst(size_t m, size_t n, const std::vector<size_t>& k, size_t p);
    // sugar syntax
    void cp(size_t m);
    void sp(size_t m, size_t n);
    void tp(size_t m);

    std::string string() const;
    std::string repr() const;
    std::string repr(size_t lno) const;
    std::string repr_new() const;
    std::string repr_new(size_t lno) const;
    std::string def_list() const;

    void read_script(const std::string& scriptname, size_t limit = -1);
    void read_script(const FileData& fdata, size_t limit = -1);

    void read_def_file(const std::string& fname);
    const Environment& env() const;
    int def_num(const std::shared_ptr<Definition>& def) const;

  private:
    Environment _env;
    std::map<std::string, int> _def_dict;
    bool _skip_check = false;
};

bool is_var_applicable(const Book& book, size_t idx, char var);
bool is_weak_applicable(const Book& book, size_t idx1, size_t idx2, char var);
bool is_form_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_appl_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_abst_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_conv_applicable(const Book& book, size_t idx1, size_t idx2);
bool is_def_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name);
bool is_def_prim_applicable(const Book& book, size_t idx1, size_t idx2, const std::string& name);
bool is_inst_applicable(const Book& book, size_t idx, size_t n, const std::vector<size_t>& k, size_t p);
bool is_tp_applicable(const Book& book, size_t idx);
