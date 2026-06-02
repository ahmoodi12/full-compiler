#pragma once 

#include "combined_include.hpp"
#include "compiler_cxt.hpp"
#include "lexer.hpp"
#include "rule_base.hpp"

class PrattParser {
public:
    enum RuleType {
        Prefix,
        Infix,
        Value,
        OpeningWrapper,
        ClosingWrapper
    };

    struct Rule : RuleBase {
        std::vector<RuleType> types;
        std::uint16_t left_power = 0;
        std::uint16_t right_power = 0;
    };

    struct ASTNode {
        Lexer::Token token;
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    std::vector<Rule> rules;
    std::pair<Rule, Rule> expr_wrapper;

    CompilerCxt& cxt;

    Lexer::Token& peek(std::span<Lexer::Token> tokens);

    std::span<Lexer::Token> consume(std::span<Lexer::Token> tokens, Lexer::Token& out);

    Rule* find_rule(const Lexer::Token& token);
    
    PrattParser(CompilerCxt& cxt, std::vector<Rule> rules);

    void load_json(json& data, const std::vector<Lexer::Rule>& lexer_rules);
    
    void run(std::vector<Lexer::Token> input);
};