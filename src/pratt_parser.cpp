#include "pratt_parser.hpp"
#include "utils.hpp"
#include "lexer.hpp"
#include "json.hpp"

using json = nlohmann::json;

bool PrattParser::has(uint32_t mask, TypeMask t) {
    return mask & (uint32_t)t;
}

std::pair<PrattParser::Rule*, PrattParser::ParseError> PrattParser::find_rule(const Token& t) {
    auto it = by_id.find(t.id);
    if (it == by_id.end()) {
        return {nullptr, {"unknown token rule: '" + t.label + "'", pos}};
    }
    return {it->second, {}};
}

bool PrattParser::valid_expr(PrattParser::ExprResult& expr) {
    return expr.error.pos == -1 || expr.error.message.empty();
}

PrattParser::PrattParser(CompilerCxt& cxt, std::vector<Rule> rules, int64_t& pos)
    : cxt(cxt), rules(std::move(rules)), pos(pos) {

    // build lookup tables
    for (auto& r : this->rules) {
        by_id[r.id] = &r;
        by_label[r.label] = &r;
    }
}

void PrattParser::load_json(json& data, Lexer& lexer) {
    rules.clear();
    by_id.clear();
    by_label.clear();

    prefix_bp = data.at("prefix binding power");

    auto& expr_data = data.at("expr definition");

    rules.reserve(expr_data.size());

    for (auto& [key, value] : expr_data.items()) {
        Rule rule;

        Lexer::Rule* lex_rule = lexer.find_lex_rule(key);
        
        if (!lex_rule) {
            utils::error("unknown lexer rule: " + key, cxt);
        } else {
            rule.id = lex_rule->id;
            rule.label = lex_rule->label;
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

PrattParser::ExprResult PrattParser::parse_atom() {
    Token tok = consume(this);
    auto rule_pair = find_rule(tok);
    Rule* rule = rule_pair.first;

    if (!rule) {
        return {.error = rule_pair.second};
    }


    if (has(rule->type_mask, Value)) {
        return {ASTNode(tok)};
    }

    if (has(rule->type_mask, Prefix)) {
        ASTNode node(tok);

        ExprResult right = parse_expr(prefix_bp);

        if (!valid_expr(right)) {
            return right;
        }
        

        node.children.push_back(
            std::make_unique<ASTNode>()
        );

        return {std::move(node)};
    }

    if (has(rule->type_mask, OpeningWrapper)) {
        ExprResult expr = parse_expr(0);
        
        if (!valid_expr(expr)) return expr;

        auto rule_pair = find_rule(consume(this));

        if (eof(this) || !rule_pair.first || !has(rule_pair.first->type_mask, ClosingWrapper)) {
            return {.error = {"expected closing wrapper", pos}};
        }

        return expr;
    }

    
    return {.error = {"invalid atom: " + tok.label, pos}};
}

PrattParser::ExprResult PrattParser::parse_expr(uint16_t rbp) {
    if (eof(this)) return {};
    ExprResult left_expr = parse_atom();

    if (!valid_expr(left_expr)) return left_expr;
    ASTNode& left = left_expr.node;

    while (!eof(this)) {
        Token& tok = peek(this);
        auto rule_pair = find_rule(tok);
        Rule* rule = rule_pair.first;

        if (!rule) return {.error = rule_pair.second};

        if (has(rule->type_mask, OpeningWrapper)) {

            consume(this); // '('

            ASTNode call;

            add_child(call, left);

            if (eof(this)) {
                return {.error = {"unclosed function call", pos}};
            }

            auto rule_pair = find_rule(peek(this));
            if (!rule_pair.first) return {.error = rule_pair.second};

            if (!has(rule_pair.first->type_mask, ClosingWrapper)) {
                while (true) {
                    ExprResult arg_expr = parse_expr(0);

                    if (!valid_expr(arg_expr)) return arg_expr;
                    ASTNode& arg = arg_expr.node;

                    add_child(call, arg);

                    if (eof(this)) {
                        return {.error = {"unclosed function call", pos}};
                    }

                    auto next_pair = find_rule(peek(this));
                    Rule* next = next_pair.first;
                    if (!next) return {.error = next_pair.second};

                    // end call
                    if (has(next->type_mask, ClosingWrapper)) {
                        consume(this);
                        break;
                    }

                    // comma / separator
                    if (has(next->type_mask, ArgSep)) {
                        consume(this);
                        continue;
                    }

                    return {.error = {"invalid function argument separator", pos}};
                }
            } else {
                // empty call, consume ')'
                consume(this);
            }

            left = std::move(call);
            continue;
        }

        if (has(rule->type_mask, Ternary)) {
            consume(this);  // "?", cond op

            ASTNode node;

            node.token = tok;

            add_child(node, left);  

            ExprResult true_stmt_expr = parse_expr(0);

            if (!valid_expr(true_stmt_expr)) return true_stmt_expr;
            ASTNode& true_stmt = true_stmt_expr.node;

            add_child(node, true_stmt);

            Token stmt_sep = consume(this);

            auto seperator_pair = find_rule(stmt_sep);
            if (!seperator_pair.first) return {.error = seperator_pair.second};

            if (!has(seperator_pair.first->type_mask, TernarySeperator)) {
                return {.error = {"ternary seperarator is invalid.", pos}};
            }
            
            ExprResult false_stmt_expr = parse_expr(0);

            if (!valid_expr(false_stmt_expr)) return false_stmt_expr;
            ASTNode& false_stmt = false_stmt_expr.node;

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
            continue;
        }

        ExprResult right_expr = parse_expr(rule->rbp);

        if (!valid_expr(right_expr)) return right_expr;
        ASTNode& right = right_expr.node;

        add_child(left, right);
    }

    return {std::move(left)};
}