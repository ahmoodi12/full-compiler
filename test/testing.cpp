
#include "lexer.hpp"
#include "utils.hpp"
#include "compiler_cxt.hpp"
#include "parser.hpp"

/*
cd builds/windows/test
ninja
*/

CompilerCxt cxt;

std::string lex_data_file = "C:/projects/full compiler/data/c frontend data/lex_data.json";
std::string parse_data_file = "C:/projects/full compiler/data/c frontend data/parser_data.json";

int main() {

    Lexer lexer(cxt, lex_data_file);
    Parser parser(cxt, parse_data_file, lexer.rules);

    std::vector<Lexer::Token> output = lexer.run(utils::read_file(utils::get_file_path("C:/projects/full compiler/test/test_file.c", cxt), cxt));

    lexer.print_output(output);

    parser.run(output);

    return 0;
}