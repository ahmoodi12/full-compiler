#pragma once 

#include "combined_include.hpp"
#include "compiler_cxt.hpp"
#include "lexer.hpp"
#include "rule_base.hpp"
#include "ast.hpp"


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

    std::vector<Rule> rules;
    Rule func_param_seperator;
    std::pair<Rule, Rule> expr_wrapper;
    uint16_t prefix_bp;

    std::span<Lexer::Token> current_set;

    CompilerCxt& cxt;

    Lexer::Token& peek();

    Lexer::Token consume();

    ASTNode parse_atom();

    bool check_type(RuleType exp_type, Rule* rule);
    bool check_type(RuleType exp_type, Lexer::Token& token);

    Rule* find_rule(const Lexer::Token& token);
    
    PrattParser(CompilerCxt& cxt, std::vector<Rule> rules);

    void load_json(json& data, const std::vector<Lexer::Rule>& lexer_rules);
    
    ASTNode parse_expr(uint16_t rbp);
};