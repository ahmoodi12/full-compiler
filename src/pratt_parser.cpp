#include "pratt_parser.hpp"
#include "lexer.hpp"
#include "utils.hpp"
#include "combined_include.hpp"

PrattParser::PrattParser(CompilerCxt& cxt, std::vector<Rule> rules)
    : cxt(cxt), rules(std::move(rules)) {}


static const Lexer::Rule* find_lex_rule(
    const std::vector<Lexer::Rule>& lexer_rules,
    const std::string& key
) {
    for (auto& r : lexer_rules) {
        if (r.label == key || std::to_string(r.id) == key)
            return &r;
    }
    return nullptr;
}

void PrattParser::load_json(json& data, const std::vector<Lexer::Rule>& lexer_rules) {
    rules.clear();
    rules.reserve(data.at("expr data").size());

    auto& wrapper = data.at("expr wrapper");

    for (auto& [key, value] : wrapper.items()) {
        if (auto* lex = find_lex_rule(lexer_rules, value); lex) {
            Rule rule;
            rule.id = lex->id;
            rule.label = lex->label;

            if (key == "closing") {
                rule.types.push_back(RuleType::ClosingWrapper);
                expr_wrapper.second = rule;
            } else if (key == "opening") {
                rule.types.push_back(RuleType::OpeningWrapper);
                expr_wrapper.first = rule;
            }
        } else {
            utils::error("wrapper key '" + key + "' not found in lexer rules", cxt);
        }
    }

    auto& expr_data = data.at("expr data");

    for (auto& [key, value] : expr_data.items()) {
        if (auto* lex = find_lex_rule(lexer_rules, key); lex) {
            Rule rule;
            rule.id = lex->id;
            rule.label = lex->label;

            const bool is_prefix = value.contains("prefix");
            const bool is_infix  = value.contains("infix");
            const bool is_value  = value.contains("value");

            if (!is_prefix && !is_infix && !is_value) {
                utils::error("rule '" + key + "' missing type (prefix/infix/value)", cxt);
            }

            if (is_prefix) rule.types.push_back(RuleType::Prefix);
            if (is_infix)  rule.types.push_back(RuleType::Infix);
            if (is_value)  rule.types.push_back(RuleType::Value);

            if (is_infix) {
                if (!value.contains("lbp") || !value.contains("rbp")) {
                    utils::error("infix rule missing lbp/rbp: " + key, cxt);
                }

                rule.left_power  = value["lbp"].get<uint8_t>();
                rule.right_power = value["rbp"].get<uint8_t>();
            }

            rules.push_back(std::move(rule));
        } else {
            utils::error("rule key '" + key + "' not found in lexer rules", cxt);
        }
    }
}

Lexer::Token& PrattParser::peek(std::span<Lexer::Token> tokens) {
    return tokens.front();
}

std::span<Lexer::Token> PrattParser::consume(std::span<Lexer::Token> tokens, Lexer::Token& out) {
    out = tokens.front();
    return tokens.subspan(1);
}

PrattParser::Rule* PrattParser::find_rule(const Lexer::Token& token) {
    for (auto& rule : rules) {
        if (rule.id == token.id) {
            return &rule;
        }
    }
}

int PrattParser::parse_expr(std::span<Lexer::Token> tokens, PrattParser::ASTNode start_node = {}) {
    /* 
    parse expr("10 + 5 * -3 - 7" nullptr) ->
    start_node-"10"-"+"  :  + wins ->
    parse expr("5 * -3 - 7", "+") -> 
    "+"-"5"-"*"  :  * wins ->
    parse expr("-3 - 7", "*") ->
    "*"-"-3"-"-"  :  - wins ->
    parse expr("7", "-") ->
    "-"-"7"-null  :  - wins

    the winner:
        make a new node and store the middle var (value) in the child of the winner as another node.
    the loser: 
        make a new node and store the winner as a child.
    */
}