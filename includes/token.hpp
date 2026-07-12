#pragma once

#include "combined_include.hpp"
#include "utils.hpp"

struct Token {
    int32_t id = -1;
    std::string label;
    std::string data;
    bool skip;
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