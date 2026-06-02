/*
# windows
cd builds/windows
ninja 

# linux
cd builds/linux
ninja 
*/

#include "cxxopts.hpp"
#include "lexer.hpp"
#include "combined_include.hpp"
#include "compiler_cxt.hpp"
#include "utils.hpp"

#include <string>

namespace argparse = cxxopts;

// for now, later this will be a part of the arguments in form of a language to choose
std::string lexer_data_file = "C:/projects/full compiler/data/c frontend data/lex_data.json";

CompilerCxt cxt;

// exit on 0
void parse_args(CompilerCxt& cxt, int argc, char **argv) {
    argparse::Options argparser("main", "llvm compiler");

    argparser.add_options()
    ("program", "program file", argparse::value<std::string>())
    ("isa", "isa file", argparse::value<std::string>())
    ("o,output", "output file", argparse::value<std::string>())
    ("w,warn", "show warnings", argparse::value<bool>()->default_value("false"))
    ("t,trace", "trace execution", argparse::value<bool>()->default_value("false"))
    ("h,help", "Show help");

    argparser.parse_positional({ "program", "isa" });
    
    auto args = argparser.parse(argc, argv);
    
    if (!args.count("program")){
        utils::error("missing program file.", cxt);
    } 

    if (!args.count("isa")){
        utils::error("missing isa file.", cxt);
    } 

    if (args.count("help")){
        std::cout << argparser.help() << std::endl;
        exit(0);
    }

    cxt.program_file = utils::get_file_path(args["program"].as<std::string>(), cxt);
    cxt.isa_file = utils::get_file_path(args["isa"].as<std::string>(), cxt);

    if (args.count("o")){
        cxt.output_file = utils::get_file_path(args["o"].as<std::string>(), cxt);
    } else {
        cxt.output_file = cxt.program_file;
        cxt.output_file.replace_extension(".asm");
    }

    cxt.current_file = cxt.program_file;
}

int main(int argc, char **argv)
{
    parse_args(cxt, argc, argv);

    Lexer lexer(cxt, lexer_data_file);
    
    lexer.run(utils::read_file(cxt.program_file, cxt));

    return 0;
}