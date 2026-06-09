#pragma once

#include "combined_include.hpp"

struct Token {
    int32_t id = -1;
    std::string label;
    std::string data;
    bool skip;
};