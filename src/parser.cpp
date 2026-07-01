#include "parser.hpp"
#include "json_validator.hpp"
#include "lexer.hpp"
#include "utils.hpp"

RuleBase make_token_base(Lexer& lexer, std::string token) {
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

void Parser::add_seq_tokens(json sequence, Sequence& item) {
    for (auto& token : sequence) {
        item.sequence.push_back(make_token_base(lexer, token));
    }
}

void Parser::parse_grammar_rule(
    json &pattern,
    Parser::Rule& rule,
    std::string& statement_str) {

    for (auto& sequence : pattern) {
        Sequence pattern_item;
        if (sequence.is_array()) {
            add_seq_tokens(sequence, pattern_item);
        } else {
            // optional tagged object
            pattern_item.optional = true;
            add_seq_tokens(sequence.at("optional"), pattern_item);
        }

    }
}


void Parser::parse_grammar_rules(
    json& grammar,
    std::vector<Parser::Rule>& rules,
    bool add_to_by_statement) {

    for (auto& [statement_str, value] : grammar.items()) {
        Rule rule;
        
        rule.pattern.push_back(Sequence{false});

        rule.statement = statement_str;

        auto& pattern = value.at("pattern");
        if (value.at("pattern").empty()) {
            utils::error("the statement rule '" + rule.statement + "' can't have a empty pattern.", cxt);
        }

        if (pattern[0].is_object()) {
            utils::error("the statement rule '" + rule.statement + " patterns first token can't be optional (assumed to be the keyword).", cxt);
        }

        parse_grammar_rule(pattern, rule, rule.statement);

        rules.push_back(rule);

        if (add_to_by_statement) by_statement[statement_str] = &grammar_rules.back();
    }
}

bool Parser::token_is_unique(Rule stmt, TokenRule token, int tok_i) {
    return std::none_of(grammar_rules.begin(), grammar_rules.end(), 
        [stmt, token, tok_i](const Parser::Rule rule){
            if (&rule == &stmt) return false;
            int i = 0;
            for (auto& seq : rule.pattern) {
                for (auto& other_token : seq.sequence) {
                    if (i++ == tok_i) {
                        return (token.label == other_token.label);
                    }
                }
            }
            return false;
    }
    );
}

int Parser::parse_commit_points(Rule stmt, int tok_i, Sequence seq){
    for (auto& token : seq.sequence) {
        if (token_is_unique(stmt, token, tok_i)) {
            token.commit_point = true;
            return tok_i;
        }
        tok_i++;
    }
    return tok_i;
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

        parse_grammar_rules(grammar.at("statement rules"), grammar_rules, true);

        parse_grammar_rules(grammar.at("variables"), variable_sub_statements, false);

        for (auto& stmt : grammar_rules) {
            int tok_i = 0;

            for (auto& seq : stmt.pattern) {
                if (seq.optional) {
                    parse_commit_points(stmt, tok_i, seq);
                } else {
                    tok_i = parse_commit_points(stmt, tok_i, seq);
                }
            }
        }

        pratt_parser.load_json(data, lexer);

        cxt.current_file = old;
    }
}


void Parser::parse_statements(std::vector<ASTNode>& output) {
    while (pos < tokens->size()) {
        StmtMatch longest_match;
        for (auto& rule : grammar_rules) {
            StmtMatch match = match_stmt(rule);
            if (match.valid && (match.size > longest_match.size)) {
                longest_match = std::move(match);
            }
        }

        if (longest_match.valid) {
            output.push_back(parse_stmt(longest_match.rule, &longest_match));
        } else {
            return;  // if no valid statement left then exit
        }
    }
}


Parser::StmtMatch Parser::match_stmt(Rule rule, bool committed) {
    size_t start_pos = pos;
    StmtMatch result;

    for (auto& pattern_item : rule.pattern) {
        size_t item_start_pos = pos;

        for (auto& exp_token : pattern_item.sequence) {
            auto token = peek(this);
            
            // commit to a path in the rule
            if (exp_token.commit_point) {
                committed = 1;
            }

            for (auto& var_rule : variable_sub_statements) {
                if (var_rule.statement == exp_token.label) {
                    StmtMatch match = match_stmt(var_rule, committed); 
                    
                    if (!match.valid) {
                        if (committed) {
                            utils::error("Expected variable segment '" + var_rule.statement + "'", cxt);
                        }
                        pos = start_pos;
                        result.valid = false;
                        return result;  
                    }

                    // Success path: bypass the variable container, collect its statements
                    result.sub_stmts.insert(result.sub_stmts.end(), 
                        std::make_move_iterator(match.sub_stmts.begin()), 
                        std::make_move_iterator(match.sub_stmts.end()));
                    
                    // Explicitly continue to the next part of your sequence loop
                    continue; 
                }
            }

            if (exp_token.label == "_expr") {
                ASTNode expr = pratt_parser.parse_expr(0);
                result.exprs.push_back(std::move(expr));

            } else if (exp_token.label == "_statements") {
                parse_statements(result.sub_stmts);

            } else if (exp_token.id != -1) {
                if (token.id != exp_token.id && !pattern_item.optional) {
                    if (committed) {
                        utils::error("expected a '" + exp_token.label + "', got a '" + token.label + "'", cxt);

                    } else if (pattern_item.optional) {
                        pos = item_start_pos;
                        break;
                        
                    } else {
                        // reset pos
                        pos = start_pos;
                        result.valid = false;
                        return result;
                    }
                }
                consume(this);
            } else {
                utils::error("unknown token '" + exp_token.label + "'" , cxt, "--- PATTERN ---" + rule.stringify_pattern());
            }
        }
    }

    // if successful then don't reset the pos
    result.valid = true;
    result.size = pos - start_pos;
    result.rule = &rule;

    return result;
}

ASTNode Parser::parse_stmt(Rule* rule, StmtMatch* match) {
    ASTNode node;
    
    

    return node;
}

std::vector<ASTNode> Parser::run(std::vector<Token>* input) {
    tokens = input;
    pratt_parser.tokens = input;
    pos = 0;
    
    std::vector<ASTNode> output;

    parse_statements(output);

    if (pos < tokens->size()) {
        utils::error("unknown grammar formation", cxt);
    }
    return output;
}