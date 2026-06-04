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

std::vector<ASTNode> Parser::run(std::vector<Token> input) {
    pratt_parser.tokens = &input;
    std::vector<ASTNode> out;

    int line_count = 0;
    while (pratt_parser.pos < input.size())
    {
        ASTNode output = pratt_parser.parse_expr(0);
        out.push_back(std::move(output));
        line_count++;
        pratt_parser.pos += 1;
    }
    return out;
}