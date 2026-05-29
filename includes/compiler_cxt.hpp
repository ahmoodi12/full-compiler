#pragma once

#include "combined_include.hpp"

class CompilerCxt {
public:
    filesystem::path program_file;
    filesystem::path isa_file;
    filesystem::path output_file;
    filesystem::path current_file;
    bool show_warnings;
    bool debug_mode;  // same as trace execution

};
