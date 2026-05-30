
#include "lexer.hpp"


/*
cd builds/windows/test
ninja
*/

CompilerCxt cxt;

string lex_data_file = "C:/projects/full compiler/data/c frontend data/lex_data.json";

int main() {

    Lexer lexer(cxt, lex_data_file);

    lexer.run(utils::read_file(utils::get_file_path("C:/projects/full compiler/test/test_file.c")));

    lexer.print_last_output();

    return 0;
}