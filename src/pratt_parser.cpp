#include "pratt_parser.hpp"
#include "lexer.hpp"
#include "utils.hpp"

PrattParser::PrattParser(CompilerCxt& cxt, vector<ParseRule> rules) : cxt(cxt), rules(rules) {}

void PrattParser::read_json(json& data, vector<Lexer::TokenRule> lexer_rules) {
    for (auto& [key, value] : data.at("expr data").items()) {
            if (any_of(lexer_rules.begin(), 
                       lexer_rules.end(), 
                       [key](auto&& rule){
                return rule.id != stoi(key) && rule.debug_name != key
            })){
                utils::error("the key '" + key + "' doesn't match any of the lexers ids or debug names", cxt);
            }

            if (!value.contains("prefix") && 
                !value.contains("infix") &&
                !value.contains("value")) {
                    utils::error("the key '" + key + "needs to contain atleast one of 'prefix', 'infix' or 'value'", cxt);
                }

            if (!value.contains("lbp") && 
                value.contains("infix") &&
                !value.contains("rbp")) {
                    utils::error("a infix needs to have a right and left binding power. the node: " + key, cxt);
                }
            
        }
}

void PrattParser::run(vector<Lexer::Token> input) {

}