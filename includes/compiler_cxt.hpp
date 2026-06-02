#pragma once

#include <filesystem>

class CompilerCxt {
public:
    std::filesystem::path program_file;
    std::filesystem::path isa_file;
    std::filesystem::path output_file;
    std::filesystem::path current_file;
    bool show_warnings;
    bool debug_mode;  // same as trace execution
};