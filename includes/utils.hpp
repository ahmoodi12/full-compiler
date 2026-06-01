// utils.hpp

#pragma once

#include "combined_include.hpp"

namespace utils {

void error(
    string prompt,
    CompilerCxt cxt = {},
    bool is_warning = false,
    bool fatal = true
);

filesystem::path get_file_path(string filename);
string read_file(const filesystem::path& path);

string visualize_whitespaces(const string& s);

}