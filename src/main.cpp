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
#include "parser.hpp"


namespace argparse = cxxopts;

CompilerCxt cxt;

std::string config_files[] = {
    "lex_config.json",
    "parse_config.json"
};

// exit on 0
void parse_args(CompilerCxt& cxt, int argc, char **argv) {
    argparse::Options argparser("main", "compiler");

    argparser.add_options()
    ("program", "program file", argparse::value<std::string>())
    ("config", "config folder", argparse::value<std::string>())
    ("o,output", "output file", argparse::value<std::string>())
    ("w,warn", "show warnings", argparse::value<bool>()->default_value("false"))
    ("t,trace", "trace execution", argparse::value<bool>()->default_value("false"))
    ("h,help", "Show help");

    argparser.parse_positional({ "program", "config" });
    
    auto args = argparser.parse(argc, argv);
    
    if (!args.count("program")){
        utils::error("missing program file.", cxt);
    } 

    if (!args.count("config")){
        utils::error("missing config folder.", cxt);
    } 

    if (args.count("help")){
        std::cout << argparser.help() << std::endl;
        exit(0);
    }

    cxt.program_files.push_back(utils::get_path(args["program"].as<std::string>(), cxt));
    cxt.program_file = &cxt.program_files.front();

    cxt.config_folder = utils::get_path(args["config"].as<std::string>(), cxt);

    if (args.count("o")){
        cxt.output_file = utils::get_path(args["o"].as<std::string>(), cxt);
    } else {
        cxt.output_file = cxt.program_files.front();
        cxt.output_file.replace_extension(".asm");
    }

    cxt.current_file = cxt.program_file;
}

int main(int argc, char **argv)
{
    parse_args(cxt, argc, argv);

    for (auto& file : config_files) {
        cxt.config_files.push_back(utils::validate_path(cxt.config_folder / file, cxt));
    }

    Lexer lexer(cxt, cxt.config_folder / "lex_config.json");
    Parser parser(cxt, cxt.config_folder / "parse_config.json", lexer);

    std::vector<Token> lex_output = lexer.run(utils::read_file(cxt.program_files.front(), cxt));

    lexer.print_output(lex_output);

    auto parse_output = parser.run(&lex_output);

    ASTPrinter().print(&parse_output);
    return 0;
}