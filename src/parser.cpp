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

Parser::StmtMatch Parser::repeat(TokenRule& repeat_token, bool use_seperator, std::function<bool(const Token&)> separator_func) {
    StmtMatch result;
    while (!eof(this)) {          
        StmtMatch match;
        match_token(match, repeat_token, peek(this));

        // if the main repeated token stops matching that means we reached the end of the repetition.
        if (!match.valid){
            result.valid = true;
            return result;  
        }

        result.sub_stmts.push_back(std::make_unique<ASTNode>(convert_match(&match)));

        if (use_seperator) {
            if (!separator_func(peek(this)))
                return {.error = {.message = "invalid seperator whilst parsing statements.", .pos = pos}};
            consume(this);
        }
    }
    return result;
}

Parser::StmtMatch Parser::try_all_statements() {
    int longest_match_i = -1;
    int longest_valid_match_i = -1;
    int longest_match_size = 0;
    int longest_valid_match_size = 0;

    std::vector<StmtMatch> matches;
    matches.reserve(grammar_rules.size());
    for (auto& rule : grammar_rules) {
        StmtMatch match = match_stmt(rule);
        if (match.size > longest_match_size) {
            longest_match_i = matches.size();
            longest_match_size = match.size;
        }

        if (match.size > longest_valid_match_size && match.valid) {
            longest_valid_match_i = matches.size(); 
            longest_valid_match_size = match.size;
        }

        matches.push_back(std::move(match));
    }

    if (longest_valid_match_i > -1) {
        StmtMatch& longest_valid_match = matches[longest_valid_match_i];
        pos += longest_valid_match.size;
        return std::move(longest_valid_match);

    } else {
        StmtMatch& longest_match = matches[longest_match_i];
        utils::error(longest_match.error.message, cxt, longest_match.error.context);
    }
}

void Parser::match_token(StmtMatch& result, TokenRule& exp_token, Token& token) {
    if (exp_token.label == "__expr__") {
        PrattParser::ExprResult expr = pratt_parser.parse_expr(0);
        if (!PrattParser::valid_expr(expr)) {
            result.error = expr.error;
            goto failed;
        }
        
        result.exprs.push_back(std::make_unique<ASTNode>(std::move(expr.node)));
        
    } else if (exp_token.label == "__stmt__") {
        StmtMatch stmt = try_all_statements();
        if (!stmt.valid) {
            result.error = stmt.error;
            goto failed;
        }

        ASTNode node = convert_match(&stmt);
        result.sub_stmts.push_back(std::make_unique<ASTNode>(node));
        if (!exp_token.capture_name.empty()) {
            result.captures[exp_token.capture_name] = std::make_unique<ASTNode>(node);
        }

    } else if (exp_token.id != -1) {
        if (token.id != exp_token.id) {
            result.error.message = "expected a '" + exp_token.label + "', got a '" + token.label + "'";
            result.error.pos = pos;
            goto failed; 
        }
        
        consume(this);
        
        if (!exp_token.capture_name.empty()) {
            result.captures[exp_token.capture_name] = std::make_unique<ASTNode>(convert_token(&token));
        }

    } else {
        bool is_var = 0;
        for (auto& var_rule : variable_sub_statements) {
            if (var_rule.statement == exp_token.label) {
                StmtMatch match = match_stmt(var_rule); 
                pos += match.size;
                
                if (!match.valid) {
                    result = std::move(match);
                    goto failed;
                }

                ASTNode node = convert_match(&match);

                result.sub_stmts.push_back(std::make_unique<ASTNode>(node));
                if (!exp_token.capture_name.empty()) {
                    result.captures[exp_token.capture_name] = std::make_unique<ASTNode>(node);
                }

                is_var = 1;
                break;
            }
        }
        if (!is_var){
            result.error.message = "unknown token '" + exp_token.label + "'"; 
            result.error.pos = pos;
            goto failed;
        }
    }

    result.valid = true;
    return;

    failed:
    result.valid = false;
    return;
}

Parser::StmtMatch Parser::match_stmt(Rule& rule) {
    size_t start_pos = pos;
    StmtMatch result;

    for (int exp_tok_i = 0; exp_tok_i < rule.pattern.size(); exp_tok_i++) {
        auto& exp_token = rule.pattern[exp_tok_i];
        if (eof(this)) {
            result.error.message = "expected '" + exp_token.label + "' got the file ended.";
            result.error.pos = pos;
            goto failed;
        }
        auto& token = peek(this);

        if (exp_token.repeat) {
            StmtMatch match = repeat(exp_token, !exp_token.seperator->label.empty(), 
                [&exp_token](const Token& token){
                    return exp_token.seperator->label == token.label;
                });
            
            result.sub_stmts.push_back(std::make_unique<ASTNode>(convert_match(&match)));
        }

        match_token(result, exp_token, token);
        if (!result.valid) goto failed;
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

ASTNode Parser::convert_match(StmtMatch* match) {
    /* node structure: 
    token label - statement
    children - exprs, sub stmts
    */

    ASTNode node;
    
    node.token.label = match->rule->statement;
    node.captures = std::move(match->captures);
    
    for (auto& expr : match->exprs) {
        add_child(node, *expr);
    }

    for (auto& stmt : match->sub_stmts) {
        add_child(node, *stmt);
    }

    return node;
}

ASTNode Parser::convert_token(Token* token) {
    /* node structure: 
    token label - statement
    children - exprs, sub stmts
    */

    ASTNode node;
    
    node.token = *token;

    return node;
}

std::vector<ASTNode> Parser::run(std::vector<Token>* input) {
    tokens = input;
    pratt_parser.tokens = input;
    pos = 0;
    
    std::vector<ASTNode> output;

    while (!eof(this)){
        StmtMatch match = try_all_statements();
        output.push_back(std::move(convert_match(&match)));
    }

    if (pos < tokens->size()) {
        utils::error("unknown grammar formation", cxt);
    }
    return output;
}