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
                "expr wrapper",
                JsonValidator::Type::Object,
                {
                    JsonValidator::Schema {
                        "(opening|closing)",
                        JsonValidator::Type::String,
                    }
                }
            },
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
                                "(value|prefix|infix|)",
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

public:
    PrattParser pratt_parser;
    JsonValidator json_validator;

    Parser(
        CompilerCxt& cxt, 
        const std::string& filename, 
        std::vector<Lexer::Rule> lexer_rules,
        std::vector<PrattParser::Rule> pratt_rules = {}
    );
};