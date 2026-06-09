#include "parser.hpp"
#include "json_validator.hpp"
#include "lexer.hpp"
#include "utils.hpp"

static RuleBase make_token_base(Lexer lexer, std::string token) {
    RuleBase token_base;
    Lexer::Rule* lex_rule = lexer.find_lex_rule(token);
    if (lex_rule) {
        token_base.id = lex_rule->id;
        token_base.label = lex_rule->label;
    } else {
        token_base.label = token;
    }
    return token_base;
}

Parser::Parser(
        CompilerCxt& cxt, 
        const std::string& filename,
        Lexer& lexer,
        std::vector<PrattParser::Rule> pratt_rules) 
        : cxt(cxt), pratt_parser(cxt, pratt_rules, pos), json_validator(cxt, json_schema), lexer(lexer) {
    if (!filename.empty()) {
        json data = load_and_validate_json(cxt, filename, json_validator);
        
        auto old = cxt.current_file;
        cxt.current_file = filename;

        json grammar = data.at("grammar");

        for (auto& statement : grammar.at("statements")) {
            statements.push_back(statement);
        }

        for (auto& [statement_str, value]: grammar.at("statement rules").items()) {
            Rule rule;
            
            bool valid = false;
            for (auto& statement : statements) {
                if (statement == statement_str) {
                    by_statement[statement_str] = &rule;
                    valid = true; 
                    break;
                }
            }

            if (!valid) utils::error("the rule '" + statement_str + "' doesn't match any defined statements.", cxt);
            
            rule.statement = statement_str;
            
            rule.pattern.push_back(PatternItem{false});
            int pattern_i = 0;

            auto& pattern = value.at("pattern");
            if (value.at("pattern").empty()) {
                utils::error("the statement rule '" + rule.statement + "' can't have a empty pattern.", cxt);
            }

            if (pattern[0].is_array()) {
                utils::error("the statement rule '" + rule.statement + " patterns first token can't be optional (assumed to be the keyword).", cxt);
            }

            rule.keyword = make_token_base(lexer, pattern[0]);

            for (auto& token : pattern) {
                if (token.is_array()) {
                    if (token[0] == "optional") {
                        rule.pattern.emplace_back(PatternItem{true});
                        pattern_i++;

                        for (size_t i = 1; i < token.size(); ++i) {
                            rule.pattern[pattern_i].sequence.emplace_back(make_token_base(lexer, token[i]));
                        }

                        rule.pattern.emplace_back(PatternItem{false});
                        pattern_i++;
                        continue;
                    } else utils::error("sub array of the statement pattern '" + statement_str + "' must be optional", cxt);
                }

                rule.pattern[pattern_i].sequence.emplace_back(make_token_base(lexer, token));
            }

            rules.push_back(rule);
        }

        pratt_parser.load_json(data, lexer);

        cxt.current_file = old;
    }
}


ASTNode* Parser::parse_stmt(Rule rule) {

}


Parser::StmtMatch Parser::match_stmt(Rule rule) {
    size_t start_pos = pos;
    StmtMatch result;
    for (auto& item : rule.pattern) {
        size_t item_start_pos = pos;

        for (auto& exp_token : item.sequence) {
            auto token = peek(this);
            
            // if token is a lexer token, no "expr" or "statements"
            if (exp_token.id != -1) {
                if (token.id != exp_token.id && !item.optional) {
                    result.valid = false;
                    result.size = pos-start_pos;
                    pos = start_pos;
                    return result;
                } else if (item.optional) {
                    // if optional and not matched then recover and continue to the next part
                    pos = item_start_pos;
                    break;
                }
            } else {
                if (exp_token.label == "expr") {
                    ASTNode expr = pratt_parser.parse_expr(0);
                    result.exprs.push_back(expr);
                }
            }

            consume(this);
        }
    }

    result.valid = true;
    result.size = pos-start_pos;
    pos = start_pos;

    return result;
}


std::vector<ASTNode> Parser::run(std::vector<Token>* input) {
    tokens = input;
    pratt_parser.tokens = input;
    pos = 0;
    
    while (true) {
        auto& keyword = peek(this);

        StmtMatch* longest_match;
        Rule* longest_rule;
        for (auto& rule : rules) {
            if (keyword.id == rule.keyword.id) {
                StmtMatch match = match_stmt(rule);
                if (match.valid && (!longest_match || match.size > longest_match->size)) {
                    longest_match = &match;
                    longest_rule = &rule;
                }
            }
        }

        if (longest_rule->statement == "if") {
            ASTNode node(consume(this), ASTNode::Type::If);

            while (consume(this).label != "expr");

            node.children.emplace_back(std::make_unique<ASTNode>(longest_match->exprs[0]));
        }

    }

}