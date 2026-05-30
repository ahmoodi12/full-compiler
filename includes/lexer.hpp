// lexer.hpp

#pragma once

// throw both cpp file includes and hpp file includes here
#include "compiler_cxt.hpp"
#include "combined_include.hpp"
#include "utils.hpp"
#include "json.hpp"
#include "json_validator.hpp"

using json = nlohmann::json;

class Lexer {
public:
    const JsonValidator::Schema lexer_json_schema {
        "",
        JsonValidator::Type::Object,
        {
            {
                "regexes",
                JsonValidator::Type::Object,
                {
                    {
                        "\\d+",
                        JsonValidator::Type::Array,
                        {
                            {.type = JsonValidator::Type::String},
                            {.type = JsonValidator::Type::Bool}

                        },
                        .is_tuple = true
                    }
                },
            },
            {
                "debug_names",
                JsonValidator::Type::Object,
                {
                    {
                        "\\d+",
                        JsonValidator::Type::String
                    }
                },
                true
            }
        }
    };

    struct Token {
        uint32_t id;
        string name; // debug
        string data;
        bool skip;
    };

    struct TokenRule {
        uint32_t id;
        regex pattern;
        bool skip;
        TokenRule(uint32_t id, string pattern, bool skip) 
        : id(id), pattern("^" + pattern), skip(skip) {}
    };

    CompilerCxt& cxt;

    vector<Token> last_output; 

    vector<TokenRule> rules;

    unordered_map<uint32_t, string> debug_names;

    JsonValidator json_validator;

    Lexer(
        CompilerCxt& cxt,
        string lex_data_file,
        vector<TokenRule> rules = {},
        unordered_map<uint32_t, string> names = {}
    );

    vector<Token> run(const string& input);

    void print_last_output() const;
};