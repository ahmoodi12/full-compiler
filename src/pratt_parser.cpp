#include "pratt_parser.hpp"
#include "lexer.hpp"
#include "utils.hpp"
#include "combined_include.hpp"
#include "parser.hpp"


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

    auto* lex = find_lex_rule(lexer_rules, data.at("func param seperator"));
    func_param_seperator = {lex->id, lex->label};

    prefix_bp = data.at("prefix binding power");

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

Lexer::Token& PrattParser::peek() {
    if (current_set.empty()) utils::error("peeking into empty token stream", cxt);
    return current_set.front();
}

Lexer::Token PrattParser::consume() {
    if (current_set.empty()) utils::error("peeking into empty token stream", cxt);
    Lexer::Token out = current_set.front();
    current_set = current_set.subspan(1);
    return out;
}


PrattParser::Rule* PrattParser::find_rule(const Lexer::Token& token) {
    for (auto& rule : rules) {
        if (rule.id == token.id) {
            return &rule;
        }
    }
    utils::error("unable to find the rule for token, id: " + std::to_string(token.id) + ", label: " + token.label, cxt, true, false);
    return nullptr;
}

bool PrattParser::check_type(RuleType exp_type, PrattParser::Rule* rule) {
    return rule ? (bool)std::count(rule->types.begin(), rule->types.end(), exp_type) : 0;
}

bool PrattParser::check_type(RuleType exp_type, Lexer::Token& token) {
    auto* rule = find_rule(token);
    return rule ? (bool)std::count(rule->types.begin(), rule->types.end(), exp_type) : 0;
}


ASTNode PrattParser::parse_atom() {
    Lexer::Token token = consume();
    auto* rule = find_rule(token);

    // literal, identifier, etc.
    if (check_type(RuleType::Value, rule)) {
        return ASTNode{
            .token = token
        };
    }

    // ( expr )
    if (check_type(RuleType::OpeningWrapper, rule)) {
        auto expr = parse_expr(0);

        Lexer::Token closing = consume();

        if (!check_type(RuleType::ClosingWrapper, closing)) {
            utils::error(
                "expected closing wrapper, got token id: " +
                std::to_string(closing.id) +
                ", label: '" + closing.label + "'",
                cxt
            );
        }

        return expr;
    }

    // prefix operator
    if (check_type(RuleType::Prefix, rule)) {

        auto operand = parse_expr(prefix_bp);

        ASTNode node;
        node.token = token;

        node.children.push_back(
            std::make_unique<ASTNode>(
                std::move(operand)
            )
        );

        return node;
    }

    utils::error(
        "expected value, wrapper, or prefix operator. got token id: " +
        std::to_string(token.id) +
        ", label: '" + token.label + "'",
        cxt
    );
    //std::unreachable();
}


ASTNode PrattParser::parse_expr(uint16_t rbp) {
    auto left = parse_atom();

    while (true) {
        auto& op = peek();
        auto* op_rule = find_rule(op);

        // function call
        if (check_type(RuleType::OpeningWrapper, op_rule)){
            consume();

            Rule* ending_token_rule = find_rule(peek());
            ASTNode node;
            node.children.push_back(std::make_unique<ASTNode>(std::move(left)));

            while (true) {
                auto arg = parse_expr(0);
                node.children.push_back(std::make_unique<ASTNode>(std::move(arg)));
                ending_token_rule = find_rule(consume());  // should be commas or seperator then closingWrapper
                
                if (check_type(RuleType::ClosingWrapper, ending_token_rule)) {
                    break;
                } else if (ending_token_rule->id != func_param_seperator.id) {
                    utils::error("the function '" + left.token.data + "' has invalid argument seperation.", cxt);
                }
            }
            
            left = std::move(node); 
            continue;
        }

        if (!check_type(RuleType::Infix, op_rule) || rbp > op_rule->left_power) {
            break;
        }

        auto sub_expr = parse_expr(op_rule->right_power);

        ASTNode node;
        node.token = op;

        node.children.push_back(
            std::make_unique<ASTNode>(std::move(left))
        );

        node.children.push_back(
            std::make_unique<ASTNode>(std::move(sub_expr))
        );


        left = std::move(node);
    }
    return left;
}