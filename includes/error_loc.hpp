#pragma once

#include <filesystem>

struct Location {
    std::filesystem::path* file;
    int line = 1;
    int column = 0;
};
