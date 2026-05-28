/*
# windows
cd builds\windows
ninja 

# linux
cd builds\linux
ninja 

*/

#include "../includes/cxxopts.hpp"
#include "../includes/utils.hpp"
#include "../includes/combined_include.hpp"
#include "../includes/compiler_cxt.hpp"

namespace argparse = cxxopts;

// for now, later this will be a part of the arguments in form of a language to choose
string lexer_data_file = "../data/c frontend data/lex_data.json";


// exit on 0
CompilerCxt parse_args(CompilerCxt& cxt, int argc, char **argv) {
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
        utils::error("missing program file.");
    } 

    if (!args.count("isa")){
        utils::error("missing isa file.");
    } 

    if (args.count("help")){
        cout << argparser.help() << endl;
        exit(0);
    }

    cxt.program_file = utils::get_file_path(args["program"].as<string>());
    cxt.isa_file = utils::get_file_path(args["isa"].as<string>());

    if (args.count("o")){
        cxt.output_file = utils::get_file_path(args["o"].as<string>());
    } else {
        cxt.output_file = cxt.program_file;
        cxt.output_file.replace_extension(".asm");
    }
}


int main(int argc, char **argv)
{
    CompilerCxt cxt;
    parse_args(cxt, argc, argv);

    
    

    return 0;
}