#pragma once

#include "token.hpp"
#include "utils.hpp"
#include <memory>
#include <vector>

struct ASTNode {
    enum class Type {
        None,

        Call,
        While,
        For,
        If,
        Else,
        Return,
        Continue,
        Break,
        Block,
        Declaration,
        Assignment,
        Expression,

        // pratt types
        Value,
        Prefix,
        Infix,
        Postfix,
        Ternary,
        OpeningWrapper,
        ClosingWrapper,
        ExprEnd,
        TernarySeparator


    };

    Token token;
    Type type;

    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode() = default;
    explicit ASTNode(Token tok, Type t)
        : type(t), token(std::move(tok)) {}
};


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