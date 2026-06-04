#include "pratt_parser.hpp"
#include "utils.hpp"
#include "lexer.hpp"
#include "json.hpp"

using json = nlohmann::json;


bool PrattParser::has(uint32_t mask, TypeMask t) {
    return mask & (uint32_t)t;
}

Token& PrattParser::peek() {
    if (eof()) utils::error("peek on empty token stream", cxt);
    return (*tokens)[pos];
}

Token PrattParser::consume() {
    if (eof()) utils::error("consume on empty token stream", cxt);
    return (*tokens)[pos++];
}

bool PrattParser::eof() {
    return tokens == nullptr || pos >= tokens->size();
}

PrattParser::Rule* PrattParser::find_rule(const Token& t) {
    auto it = by_id.find(t.id);
    if (it == by_id.end()) {
        utils::error("unknown token rule: " + t.label, cxt);
        return nullptr;
    }
    return it->second;
}


PrattParser::PrattParser(CompilerCxt& cxt, std::vector<Rule> rules)
    : cxt(cxt), rules(std::move(rules)) {

    // build lookup tables
    for (auto& r : this->rules) {
        by_id[r.id] = &r;
        by_label[r.label] = &r;
    }
}

void PrattParser::load_json(json& data, const std::vector<Lexer::Rule>& lexer_rules) {
    rules.clear();
    by_id.clear();
    by_label.clear();

    prefix_bp = data.at("prefix binding power");

    auto& expr_data = data.at("expr data");

    rules.reserve(expr_data.size());

    for (auto& [key, value] : expr_data.items()) {
        Rule rule;

        // map lexer rule
        bool found = false;
        for (auto& lex : lexer_rules) {
            if (lex.label == key || std::to_string(lex.id) == key) {
                rule.id = lex.id;
                rule.label = lex.label;
                found = true;
                break;
            }
        }

        if (!found) {
            utils::error("unknown lexer rule: " + key, cxt);
        }

        // types → bitmask
        if (!value.contains("types")) {
            utils::error("rule missing types: " + key, cxt);
        }

        for (auto& type : value["types"]) {
            std::string t = type;

            if (t == "value") rule.type_mask |= Value;
            else if (t == "prefix") rule.type_mask |= Prefix;
            else if (t == "infix") rule.type_mask |= Infix;
            else if (t == "postfix") rule.type_mask |= Postfix;
            else if (t == "ternary") rule.type_mask |= Ternary;
            else if (t == "opening wrapper") rule.type_mask |= OpeningWrapper;
            else if (t == "closing wrapper") rule.type_mask |= ClosingWrapper;
            else if (t == "expr terminator") rule.type_mask |= ExprEnd;
        }

        // precedence
        if (has(rule.type_mask, Infix)) {
            if (value.contains("lbp") && value.contains("rbp")) {
                rule.lbp = value["lbp"];
                rule.rbp = value["rbp"];
            } else if (value.contains("precedence") && value.contains("associativity")) {
                uint16_t p = value["precedence"];
                std::string assoc = value.value("associativity", "left");

                rule.lbp = p;
                rule.rbp = (assoc == "right") ? p - 1 : p + 1;
            } else {
                utils::error("infix missing precedence or associativity: " + key, cxt);
            }
        }

        rules.push_back(rule);
    }

    // rebuild lookup tables
    for (auto& r : rules) {
        by_id[r.id] = &r;
        by_label[r.label] = &r;
    }
}

ASTNode PrattParser::parse_atom() {
    Token tok = consume();
    Rule* rule = find_rule(tok);

    if (!rule) utils::error("null rule in atom", cxt);

    if (has(rule->type_mask, Value)) {
        return ASTNode(tok);
    }

    if (has(rule->type_mask, Prefix)) {
        ASTNode node(tok);

        node.children.push_back(
            std::make_unique<ASTNode>(parse_expr(prefix_bp))
        );

        return node;
    }

    if (has(rule->type_mask, OpeningWrapper)) {
        ASTNode expr = parse_expr(0);

        if (eof() || !has(find_rule(consume())->type_mask, ClosingWrapper)) {
            utils::error("expected closing wrapper", cxt);
        }

        return expr;
    }

    utils::error("invalid atom: " + tok.label, cxt);
    return {};
}

ASTNode PrattParser::parse_expr(uint16_t rbp) {
    if (eof()) return {};
    ASTNode left = parse_atom();

    while (!eof()) {
        Token& tok = peek();
        Rule* rule = find_rule(tok);

        if (!rule) break;

        // ----------------------------
        // FUNCTION CALL: f(...)
        // ----------------------------
        if (has(rule->type_mask, OpeningWrapper)) {

            consume(); // '('

            ASTNode call;
            call.token.label = "CALL";

            call.children.push_back(
                std::make_unique<ASTNode>(std::move(left))
            );

            // empty call
            if (!eof() && !has(find_rule(peek())->type_mask, ClosingWrapper)) {

                while (true) {
                    call.children.push_back(
                        std::make_unique<ASTNode>(parse_expr(0))
                    );

                    if (eof()) {
                        utils::error("unclosed function call", cxt);
                    }

                    Rule* next = find_rule(peek());

                    // end call
                    if (has(next->type_mask, ClosingWrapper)) {
                        consume();
                        break;
                    }

                    // comma / separator
                    if (has(next->type_mask, ExprEnd)) {
                        consume();
                        continue;
                    }

                    utils::error("invalid function argument separator", cxt);
                }
            } else {
                // consume ')'
                consume();
            }

            left = std::move(call);
            continue;
        }

        // ----------------------------
        // INFIX STOP CONDITION
        // ----------------------------
        if (!has(rule->type_mask, Infix) || rule->lbp <= rbp) break;

        Token op = consume();

        ASTNode right = parse_expr(rule->rbp);

        ASTNode node;
        node.token = op;

        node.children.push_back(
            std::make_unique<ASTNode>(std::move(left))
        );

        node.children.push_back(
            std::make_unique<ASTNode>(std::move(right))
        );

        left = std::move(node);
    }

    return left;
}