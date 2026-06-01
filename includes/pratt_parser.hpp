#pragma once 

#include "combined_include.hpp"
#include "compiler_cxt.hpp"
#include "lexer.hpp"



class PrattParser {
public:
    struct ParseRule {
        uint32_t id;
        uint8_t left_power;
        uint8_t right_power;
        string type;
        string debug_name;
    };

    vector<ParseRule> rules;
    CompilerCxt& cxt;

    PrattParser(CompilerCxt& cxt, vector<ParseRule> rules);

    void read_json(json& data, vector<Lexer::TokenRule> lexer_rules);

    void run(vector<Lexer::Token> input);
};