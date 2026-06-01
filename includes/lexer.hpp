// lexer.hpp

#pragma once

#include "json.hpp"

class JsonValidator;

using json = nlohmann::json;

class Lexer {
public:
    const JsonValidator::Schema json_schema {
        "",
        JsonValidator::Type::Object,
        {
            JsonValidator::Schema {
                "regexes",
                JsonValidator::Type::Object,
                {
                    JsonValidator::Schema {
                        "\\d+",
                        JsonValidator::Type::Array,
                        {
                            JsonValidator::Schema{"", JsonValidator::Type::String},
                            JsonValidator::Schema{"", JsonValidator::Type::Bool}
                        },
                        false,
                        true
                    }
                }
            },
            JsonValidator::Schema {
                "debug_names",
                JsonValidator::Type::Object,
                {
                    JsonValidator::Schema{"\\d+", JsonValidator::Type::String}
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