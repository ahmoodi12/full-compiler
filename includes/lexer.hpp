#pragma once

#include "json.hpp"
#include "json_validator.hpp"
#include "rule_base.hpp"
#include "combined_include.hpp"
#include "token.hpp"

#include <regex>
#include <string>
#include <vector>

using json = nlohmann::json;

class Lexer {
public:
    const JsonValidator::Schema json_schema {
        "",
        JsonValidator::Type::Object,
        {
            JsonValidator::Schema {
                "rules",
                JsonValidator::Type::Object,
                {
                    JsonValidator::Schema {
                        "\\d+",
                        JsonValidator::Type::Array,
                        {
                            JsonValidator::Schema{"", JsonValidator::Type::String},
                            JsonValidator::Schema{"", JsonValidator::Type::String},
                            JsonValidator::Schema{"", JsonValidator::Type::Bool},
                        },
                        false,
                        true
                    }
                }
            },
        }
    };

    struct Rule : RuleBase {
        std::regex pattern;
        bool skip;

        Rule(
            std::uint32_t id,
            std::string label,
            std::string pattern,
            bool skip_
        )
            : RuleBase(id, std::move(label)),
              pattern("^" + pattern),
              skip(skip_)
        {}
    };

    CompilerCxt& cxt;
    std::vector<Rule> rules;
    JsonValidator json_validator;

    Lexer(
        CompilerCxt& cxt,
        std::string lex_data_file,
        std::vector<Rule> rules = {}
    );

    std::vector<Token> run(const std::string& input);
    void print_output(std::vector<Token> output) const;
};