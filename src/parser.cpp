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
        
        auto old = cxt.current_file;
        cxt.current_file = filename;

        pratt_parser.load_json(data.at("pratt parser"), lexer_rules);

        cxt.current_file = old;
    }
}

ASTNode Parser::run(std::vector<Lexer::Token> input) {
    pratt_parser.current_set = std::span<Lexer::Token>(input);
    return pratt_parser.parse_expr(0);
}