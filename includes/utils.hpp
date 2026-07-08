#pragma once

#include "combined_include.hpp"

class CompilerCxt;

namespace utils {

void error(
    std::string prompt,
    CompilerCxt& cxt,
    std::string note_info = "",
    bool is_warning = false,
    bool fatal = true
);

std::filesystem::path get_file_path(std::string filename, CompilerCxt& cxt);
std::string read_file(const std::filesystem::path& path, CompilerCxt& cxt);

json& json_get(json &parent, const char *item, CompilerCxt &cxt);

std::string visualize_whitespaces(const std::string& s);

}