#pragma once

#include "pratt_parser.hpp"

class JsonValidator;
class lexer;

class Parser {
    JsonValidator::Schema json_schema {
        "",
        JsonValidator::Type::Object,
        {
            JsonValidator::Schema {
                "expr data",
                JsonValidator::Type::Object,
                {
                    JsonValidator::Schema {
                        ".+",
                        JsonValidator::Type::Object,
                        {
                            JsonValidator::Schema {
                                "[rl]bp",
                                JsonValidator::Type::Int,
                                {{}},
                                true
                            },
                            JsonValidator::Schema {
                                "(value|prefix|infix)",
                                JsonValidator::Type::String,
                                {{}},
                                true
                            },
                        }
                    }
                }
            },

        }
    };

    CompilerCxt& cxt;
    json data_json;
    PrattParser pratt_parser;
    JsonValidator json_validator;

    Parser(CompilerCxt& cxt, const string& filename, vector<Lexer::TokenRule> lexer_rules, vector<PrattParser::ParseRule> pratt_rules);
};