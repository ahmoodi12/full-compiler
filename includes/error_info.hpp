#pragma once

#include "combined_include.hpp"

struct Location {
    std::filesystem::path* file;
    int line = 1;
    int column = 0;
};

// TODO: add recursive context for errors (like a stack trace)
struct ErrorInfo {
    std::string message;
    Location loc = {nullptr, -1, -1};
    std::vector<ErrorInfo> context;
};

