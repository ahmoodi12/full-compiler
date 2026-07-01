#pragma once

#include "token.hpp"
#include "utils.hpp"
#include <memory>
#include <vector>

struct ASTNode {
    Token token;

    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode() = default;
    explicit ASTNode(Token tok)
        : token(std::move(tok)) {}
};

inline void add_child(ASTNode& node, ASTNode& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}
inline void add_child(ASTNode& node, ASTNode&& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}

template<typename T>
bool eof(T* parser) {
    return parser->tokens == nullptr || parser->pos >= parser->tokens->size();
}

template<typename T>
Token& peek(T* parser) {
    if (eof(parser))
        utils::error("peek on empty token stream", parser->cxt);

    return (*parser->tokens)[parser->pos];
}

template<typename T>
Token consume(T* parser) {
    if (eof(parser))
        utils::error("consume on empty token stream", parser->cxt);

    return (*parser->tokens)[parser->pos++];
}