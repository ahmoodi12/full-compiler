#include "parser.hpp"
#include "json_validator.hpp"
#include "lexer.hpp"
#include "utils.hpp"

Parser::Parser(
        CompilerCxt& cxt, 
        const std::string& filename,
        std::vector<Lexer::Rule> lexer_rules,
        std::vector<PrattParser::Rule> pratt_rules) 
        : cxt(cxt), pratt_parser(cxt, pratt_rules), json_validator(cxt, json_schema) {
    if (!filename.empty()) {
        json data = load_and_validate_json(cxt, filename, json_validator);
        
        pratt_parser.load_json(data, lexer_rules);
    }
}