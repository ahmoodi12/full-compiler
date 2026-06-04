#pragma once

#include "lexer.hpp"
#include <memory>
#include <vector>

struct ASTNode {
    Lexer::Token token;
    std::vector<std::unique_ptr<ASTNode>> children;
};