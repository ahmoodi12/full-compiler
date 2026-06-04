#pragma once

#include "token.hpp"
#include <memory>
#include <vector>

struct ASTNode {
    Token token;
    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode() = default;
    explicit ASTNode(Token t) : token(std::move(t)) {}
};