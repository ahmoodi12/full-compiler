#pragma once

#include <filesystem>

class CompilerCxt {
public:
    std::filesystem::path program_file;
    std::filesystem::path isa_file;
    std::filesystem::path output_file;
    std::filesystem::path current_file;
    bool show_warnings = 1;
    bool debug_mode = 0;  // same as trace execution
};