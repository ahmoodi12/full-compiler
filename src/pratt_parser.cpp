#include "pratt_parser.hpp"
#include "utils.hpp"
#include "lexer.hpp"
#include "json.hpp"

using json = nlohmann::json;

bool PrattParser::has(uint32_t mask, TypeMask t) {
    return mask & (uint32_t)t;
}


PrattParser::Rule* PrattParser::find_rule(const Token& t) {
    auto it = by_id.find(t.id);
    if (it == by_id.end()) {
        utils::error("unknown token rule: " + t.label, cxt);
        return nullptr;
    }
    return it->second;
}


PrattParser::PrattParser(CompilerCxt& cxt, std::vector<Rule> rules, size_t& pos)
    : cxt(cxt), rules(std::move(rules)), pos(pos) {

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

    auto& expr_data = data.at("expr definition");

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
            if (auto it = type_map.find(type); it != type_map.end())
                rule.type_mask |= it->second;
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
    Token tok = consume(this);
    Rule* rule = find_rule(tok);

    if (!rule) utils::error("null rule in atom", cxt);

    if (has(rule->type_mask, Value)) {
        return ASTNode(tok, ASTNode::Type::Value);
    }

    if (has(rule->type_mask, Prefix)) {
        ASTNode node(tok, ASTNode::Type::Prefix);

        node.children.push_back(
            std::make_unique<ASTNode>(parse_expr(prefix_bp))
        );

        return node;
    }

    if (has(rule->type_mask, OpeningWrapper)) {
        ASTNode expr = parse_expr(0);

        if (eof(this) || !has(find_rule(consume(this))->type_mask, ClosingWrapper)) {
            utils::error("expected closing wrapper", cxt);
        }

        return expr;
    }

    utils::error("invalid atom: " + tok.label, cxt);
    return {};
}

void add_child(ASTNode& node, ASTNode& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}
void add_child(ASTNode& node, ASTNode&& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}


ASTNode PrattParser::parse_expr(uint16_t rbp) {
    if (eof(this)) return {};
    ASTNode left = parse_atom();

    while (!eof(this)) {
        Token& tok = peek(this);
        Rule* rule = find_rule(tok);

        if (!rule) break;

        // ----------------------------
        // FUNCTION CALL: f(...)
        // ----------------------------
        if (has(rule->type_mask, OpeningWrapper)) {

            consume(this); // '('

            ASTNode call;
            call.type = ASTNode::Type::Call;

            add_child(call, left);

            // empty call
            if (!eof(this) && !has(find_rule(peek(this))->type_mask, ClosingWrapper)) {

                while (true) {
                    add_child(call, parse_expr(0));

                    if (eof(this)) {
                        utils::error("unclosed function call", cxt);
                    }

                    Rule* next = find_rule(peek(this));

                    // end call
                    if (has(next->type_mask, ClosingWrapper)) {
                        consume(this);
                        break;
                    }

                    // comma / separator
                    if (has(next->type_mask, ExprEnd)) {
                        consume(this);
                        continue;
                    }

                    utils::error("invalid function argument separator", cxt);
                }
            } else {
                // consume ')'
                consume(this);
            }

            left = std::move(call);
            continue;
        }

        if (has(rule->type_mask, Ternary)) {
            consume(this);  // "?", cond op

            ASTNode node;

            node.token = tok;
            node.type = ASTNode::Type::Ternary;

            add_child(node, left);  

            ASTNode true_stmt = parse_expr(0);

            add_child(node, true_stmt);

            Token stmt_sep = consume(this);

            if (!has(find_rule(stmt_sep)->type_mask, TernarySeperator)) {
                utils::error("ternary seperarator is invalid.", cxt);}
            
            ASTNode false_stmt = parse_expr(0);

            add_child(node, false_stmt);

            left = std::move(node);
                
            continue;
        }
        
        if (!has(rule->type_mask, Infix) || rule->lbp <= rbp) break;

        Token op = consume(this);

        ASTNode node;
        node.token = op;

        add_child(node, left);

        left = std::move(node);

        if (has(rule->type_mask, Postfix)) {
            left.type = ASTNode::Type::Postfix;
            continue;
        }

        left.type = ASTNode::Type::Infix;
        ASTNode right = parse_expr(rule->rbp);

        add_child(left, right);
    }

    return left;
}