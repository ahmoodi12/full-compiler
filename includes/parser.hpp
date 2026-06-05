#pragma once

#include "pratt_parser.hpp"
#include "ansi_colors.hpp"
#include "ast.hpp"

class JsonValidator;
class lexer;

class Parser {
    JsonValidator::Schema json_schema {
        "pratt parser",
        JsonValidator::Type::Object,
        {
            JsonValidator::Schema {
                "prefix binding power",
                JsonValidator::Type::Int
            },
            JsonValidator::Schema {
                "expr data",
                JsonValidator::Type::Object,
                {
                    JsonValidator::Schema {
                        ".+",
                        JsonValidator::Type::Object,
                        {
                            JsonValidator::Schema {
                                "([rl]bp|precedence)",
                                JsonValidator::Type::Int,
                                {{}},
                                true
                            },
                            JsonValidator::Schema {
                                "associativity",
                                JsonValidator::Type::String,
                                {{}},
                                true
                            },
                            JsonValidator::Schema {
                                "types",
                                JsonValidator::Type::Array,
                                {
                                    JsonValidator::Schema {
                                        "(value|prefix|infix|callabe|expr terminator|opening wrapper|closing wrapper|ternary seperator)"
                                    }
                                }
                            }
                        }
                    }
                }
            },

        }
    };

    CompilerCxt& cxt;

public:
    PrattParser pratt_parser;
    JsonValidator json_validator;

    Parser(
        CompilerCxt& cxt, 
        const std::string& filename, 
        std::vector<Lexer::Rule> lexer_rules,
        std::vector<PrattParser::Rule> pratt_rules = {}
    );

    std::vector<ASTNode> run(std::vector<Token> input);
};



class ASTPrinter {
public:
    static void print(const ASTNode* root) {
        using namespace ansiColors;

        std::cout << bold << cyan
                  << "\n===== AST =====\n"
                  << reset;

        if (!root) {
            std::cout << red << "empty AST\n" << reset;
            return;
        }

        print_node(root, 0, true);

        std::cout << bold << cyan
                  << "===============\n"
                  << reset;
    }

private:
    static void print_node(const ASTNode* node, int depth, bool is_last) {
        using namespace ansiColors;

        if (!node) return;

        // indentation + tree structure
        for (int i = 0; i < depth-1; i++) {
            std::cout << "|   ";
        }

        if (depth > 0) {
            std::cout << (is_last ? "'- " : "|-- ");
        }

        // node info
        std::cout
            << bright_yellow << node->token.id << reset << " "
            << bright_green  << node->token.label << reset << " "
            << bright_white  << node->token.data << reset
            << "\n";

        const auto& kids = node->children;
        for (size_t i = 0; i < kids.size(); i++) {
            print_node(kids[i].get(), depth + 1, i + 1 == kids.size());
        }
    }
};