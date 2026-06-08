#include "parser.hpp"
#include "json_validator.hpp"
#include "lexer.hpp"
#include "utils.hpp"

Parser::Parser(
        CompilerCxt& cxt, 
        const std::string& filename,
        std::vector<Lexer::Rule> lexer_rules,
        std::vector<PrattParser::Rule> pratt_rules) 
        : cxt(cxt), pratt_parser(cxt, pratt_rules, pos), json_validator(cxt, json_schema) {
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
                if (statement == statement_str) {valid = true; break;}
            }

            if (!valid) utils::error("the rule '" + statement_str + "' doesn't match any defined statements.", cxt);
            
            rule.statement = statement_str;
            
            rule.pattern.push_back(PatternItem{false});
            int pattern_i = 0;

            for (auto& token : value.at("pattern")) {
                if (token.is_array()) {
                    if (token[0] == "optional") {
                        rule.pattern.push_back(PatternItem{true});
                        pattern_i++;

                        for (size_t i = 1; i < token.size(); ++i) {
                            auto& sub_token = token[i];
                            rule.pattern[pattern_i].sequence.emplace_back(sub_token);
                        }

                        rule.pattern.push_back(PatternItem{false});
                        pattern_i++;
                        continue;
                    } else utils::error("sub array of the statement pattern '" + statement_str + "' must be optional", cxt);
                }

                rule.pattern[pattern_i].sequence.push_back(token);
            }

            rules.push_back(rule);
        }

        pratt_parser.load_json(data, lexer_rules);

        cxt.current_file = old;
    }
}


std::vector<ASTNode> Parser::run(std::vector<Token>* input) {
    tokens = input;
    pratt_parser.tokens = input;
    pos = 0;
    
    Token first = peek(this);


}