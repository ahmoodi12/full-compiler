#include "parser.hpp"
#include "json_validator.hpp"
#include "lexer.hpp"
#include "utils.hpp"

Parser::TokenRule make_token_base(Lexer& lexer, std::string token) {
    Parser::TokenRule token_base;
    Lexer::Rule* lex_rule = lexer.find_lex_rule(token);
    if (lex_rule) {
        token_base.id = lex_rule->id;
        token_base.label = lex_rule->label;
    } else {
        token_base.label = token;
    }
    return token_base;
}

void Parser::add_seq_tokens(json& sequence, Parser::Rule& rule) {
    for (auto& token : sequence) {
        if (token.is_object()) {
            for (auto& [capture_label, capture_token] : token.items()) {
                TokenRule token_rule = make_token_base(lexer, capture_token);
                token_rule.capture_name = capture_label;
                
                rule.pattern.push_back(std::move(token_rule));
            }
        } else {
            rule.pattern.push_back(std::move(make_token_base(lexer, token)));
        }
    }
}

void Parser::parse_grammar_rule(
    json& pattern,
    const std::string& statement_str,
    std::vector<Parser::Rule>& rules,
    bool allow_optionals, 
    int seq_i = 0) {
    int rule_i = rules.size() - 1;
        
    for (; seq_i < pattern.size(); seq_i++) {
        json& sequence = pattern[seq_i];
        Rule& rule = rules[rule_i];

        if (sequence.is_array()) {
            add_seq_tokens(sequence, rule);
        } else if (sequence.contains("repeat")) {
            TokenRule token_rule = make_token_base(lexer, sequence.at("repeat"));
            token_rule.repeat = 1;
            if (sequence.contains("seperator")) {
                token_rule.seperator = std::make_unique<TokenRule>(make_token_base(lexer, sequence.at("seperator")));
            }

            rule.pattern.push_back(std::move(token_rule));

        } else {
            // optional path
            if (!allow_optionals) {
                utils::error("optionals not allowed in variables.", cxt);
            }

            Rule optional_path;

            optional_path.statement = statement_str;

            for (auto& token : rule.pattern) {
                optional_path.pattern.push_back(std::move(token));
            }
            
            add_seq_tokens(pattern[seq_i].at("optional"), optional_path);
            
            optional_path.parent_i = rule_i;
            
            rules.push_back(std::move(optional_path));

            parse_grammar_rule(pattern, statement_str, rules, allow_optionals, seq_i + 1);
        }
    }
}


void Parser::parse_grammar_rules(
    json& grammar,
    std::vector<Parser::Rule>& rules,
    bool is_grammar_rules) {

    for (auto& [statement_str, value] : grammar.items()) {
        Rule rule{statement_str};
        
        auto& pattern = value.at("pattern");
        if (pattern.empty()) {
            utils::error("the statement rule '" + rule.statement + "' can't have a empty pattern.", cxt);
        }

        if (pattern[0].is_object()) {
            utils::error("the statement rule '" + rule.statement + " patterns first token can't be optional (assumed to be the keyword).", cxt);
        }

        rules.push_back(std::move(rule));

        parse_grammar_rule(pattern, statement_str, rules, is_grammar_rules);

        if (is_grammar_rules) by_statement[statement_str] = &grammar_rules.back();
    }

    for (auto& rule : rules) {
        for (auto& token : rule.pattern) {
            for (auto& other_rule : rules) {
                if (other_rule.statement == token.label) {
                    token.stmts.push_back(&other_rule);
                }
            }
        }
    }
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

        parse_grammar_rules(utils::json_get(grammar, "statement rules", cxt), grammar_rules, true);

        parse_grammar_rules(utils::json_get(grammar, "variables", cxt), variable_sub_statements, false);
        
        pratt_parser.load_json(data, lexer);

        cxt.current_file = old;
    }
}

Parser::StmtMatch Parser::repeat(std::function<StmtMatch()> match_func, std::function<bool(const Token&)> stop, bool use_seperator, std::function<bool(const Token&)> separator) {
    while (pos < tokens->size()) {    
        StmtMatch match = match_func();

        bool Eof = eof(this);
        if (!Eof && stop(peek(this))) {
            return {.valid = true};

        } else if (Eof) {
            return {};
        } 
        
        if (!match.valid){
            return std::move(match);  
        }

        if (use_seperator) {
            if (!separator(peek(this)))
                return {.error = {.message = "invalid seperator whilst parsing statements.", .pos = pos}};
            consume(this);
        }
    }
}

Parser::StmtMatch Parser::parse_statement() {
    while (pos < tokens->size()) {
        int longest_match_i = -1;
        int longest_valid_match_i = -1;
        std::vector<StmtMatch> matches;
        matches.reserve(grammar_rules.size());
        for (auto& rule : grammar_rules) {
            matches.push_back(match_stmt(rule));
            StmtMatch& match = matches.back();

            if (longest_match_i == -1 || match.size > matches[longest_match_i].size) {
                longest_match_i = matches.size() - 1;
            }

            if ((longest_valid_match_i == -1 || match.size > matches[longest_valid_match_i].size) && match.valid) {
                longest_valid_match_i = matches.size() - 1; 
            }
        }

        StmtMatch& longest_match = matches[longest_match_i];


        

    }
    return {};
}


Parser::StmtMatch Parser::match_stmt(Rule& rule) {
    size_t start_pos = pos;
    StmtMatch result;
    StmtMatch stmts_result;

    for (int exp_tok_i = 0; exp_tok_i < rule.pattern.size(); exp_tok_i++) {
        auto& exp_token = rule.pattern[exp_tok_i];
        if (eof(this)) {
            result.error.message = "expected '" + exp_token.label + "' got the file ended.";
            result.error.pos = pos;
            goto failed;
        }
        auto& token = peek(this);

        bool is_var = 0;
        for (auto& var_rule : variable_sub_statements) {
            if (var_rule.statement == exp_token.label) {
                StmtMatch match = match_stmt(var_rule); 
                pos += match.size;
                
                if (!match.valid) {
                    result = std::move(match);
                    goto failed;
                }

                result.sub_stmts.push_back(parse_stmt(&match));
                is_var = 1;
                break;
            }
        }
        if (is_var) continue;

        if (exp_token.label == "__expr__") {
            PrattParser::ExprResult expr = pratt_parser.parse_expr(0);
            if (!PrattParser::valid_expr(expr)) {
                result.error = expr.error;
                goto failed;
            }
            
            result.exprs.push_back(std::move(expr.node));
            
        } else if (exp_token.label == "__statements__") {
            RuleBase& terminator = rule.pattern[exp_tok_i + 1]; 
            stmts_result = parse_statements(result.sub_stmts, 0, [terminator](const Token& token){return token.id == terminator.id;});
            if (!stmts_result.valid) {
                result.error = stmts_result.error;
                goto failed;
            }

        } else if (exp_token.id != -1) {
            if (token.id != exp_token.id) {
                result.error.message = "expected a '" + exp_token.label + "', got a '" + token.label + "'";
                result.error.pos = pos;
                goto failed;
            }
            
            consume(this);
            
            if (!exp_token.capture_name.empty()) {
                result.captures[exp_token.capture_name] = &token;
            }

        } else {
            result.error.message = "unknown token '" + exp_token.label + "'"; 
            result.error.context = "--- PATTERN ---\n" + rule.stringify_pattern();
            result.error.pos = pos;
            goto failed;
        }
    }

    // if successful then don't reset the pos
    result.valid = true;
    goto ret;

    failed:
    result.valid = false;

    ret:
    result.size = pos - start_pos;
    pos = start_pos;
    result.rule = &rule;

    return result; 
}

ASTNode Parser::parse_stmt(StmtMatch* match) {
    /* node structure: 
    token label - statement
    children - exprs, sub stmts
    */

    ASTNode node;
    
    node.token.label = match->rule->statement;
    node.captures = std::move(match->captures);
    
    for (auto& expr : match->exprs) {
        add_child(node, expr);
    }

    for (auto& stmt : match->sub_stmts) {
        add_child(node, stmt);
    }

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