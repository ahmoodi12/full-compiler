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
    using S = JsonValidator<json>::Schema;
    using T = JsonType;
    const S json_schema {
        "",
        T::Object,
        {
            S {
                "tokens",
                T::Object,
                {
                    S {
                        "\\d+",
                        T::Array,
                        {
                            S{"", T::String},
                            S{"", T::String},
                            S{"", T::Bool},
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
    JsonValidator<json> json_validator;

    Lexer(
        CompilerCxt& cxt,
        std::filesystem::path file,
        std::vector<Rule> rules = {}
    );

    std::vector<Token> run(const std::string& input);

    Rule* find_lex_rule(std::string key);

    void print_output(std::vector<Token> output) const;
};