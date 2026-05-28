// lexer.hpp

#pragma once

// throw both cpp file includes and hpp file includes here
#include "compiler_cxt.hpp"
#include "combined_include.hpp"
#include "utils.hpp"
#include "json.hpp"

using json = nlohmann::json;

struct Token {
    uint32_t id;
    string name; // debug
    string data;
};

struct TokenRule {
    uint32_t id;
    regex pattern;
};

class Lexer {
public:
    CompilerCxt cxt;

    vector<Token> last_output; 

    vector<TokenRule> rules;

    unordered_map<uint32_t, string> id_names;

    Lexer(
        CompilerCxt cxt,
        filesystem::path lex_data_file = "",
        vector<TokenRule> rules = {},
        unordered_map<uint32_t, string> names = {}
    );

    void run(const string& input);
};