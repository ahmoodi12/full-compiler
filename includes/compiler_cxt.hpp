#pragma once

#include <filesystem>

class CompilerCxt {
public:
    std::vector<std::filesystem::path> program_files;
    std::filesystem::path* program_file = nullptr;
    std::filesystem::path config_folder;
    std::vector<std::filesystem::path> config_files;
    std::filesystem::path output_file;
    std::filesystem::path* current_file = nullptr;
    bool show_warnings = 1;
    bool debug_mode = 0;  // same as trace execution
};