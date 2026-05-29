// utils.hpp

#pragma once

#include "combined_include.hpp"
#include "ansi_colors.hpp"

namespace utils {

void error(
    string prompt,
    string filename = "",
    bool show_warnings = false,
    bool is_warning = false,
    bool fatal = true
);

filesystem::path get_file_path(string filename);
string read_file(const filesystem::path& path);

}