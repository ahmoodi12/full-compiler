#pragma once

#include "pratt_parser.hpp"
#include "ansi_colors.hpp"
#include "ast.hpp"
#include "json_validator.hpp"
#include "lexer.hpp"

class CompilerCxt;

class Parser {
    using S = JsonValidator::Schema;
    using T = JsonValidator::Type;
    S json_schema{
        "",
        T::Object,
        {
            {"prefix binding power", T::Int},

            {"expr definition", T::Object, {
                {".+", T::Object, {
                    {"([rl]bp|precedence)", T::Int, {}, true},
                    {"associativity", T::String, {}, true},
                    {"types", T::Array, {
                        {"(value|prefix|infix|expr terminator|opening wrapper|closing wrapper|ternary seperator)", T::String}
                    }}
                }}
            }},

            {"grammar", T::Object, {
                {"statements", T::Array, {
                    {".+", T::String}
                }},

                {"statement rules", T::Object, {
                    {".+", T::Object, {
                        {"pattern", T::Array, {
                            {".+", T::String, {}, true},
                            {".+", T::Array, {
                                {".+", T::String, {}, true}
                            }, true}
                        }}
                    }}
                }}
            }}
        }
    };

public:
    struct PatternItem {
        bool optional = false;
        std::vector<RuleBase> sequence;
    };

    struct Rule {
        std::string statement;
        RuleBase keyword;
        std::vector<PatternItem> pattern;
    };

    struct StmtMatch {
        bool valid;
        int size;
        std::vector<ASTNode> exprs; // ONLY expr results
    };
        
    PrattParser pratt_parser;
    JsonValidator json_validator;

    std::vector<std::string> statements;
    Lexer& lexer;

    std::vector<Rule> rules;
    std::unordered_map<std::string, Rule*> by_statement;

    size_t pos = 0;
    std::vector<Token>* tokens = nullptr;

    CompilerCxt& cxt;

    Parser(
            CompilerCxt& cxt, 
            const std::string& filename,
            Lexer& lexer,
            std::vector<PrattParser::Rule> pratt_rules = {});

    ASTNode* parse_stmt(Rule rule);

    StmtMatch match_stmt(Rule rule);

    std::vector<ASTNode> run(std::vector<Token> *input);
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
    static const char* type_name(ASTNode::Type type) {
        switch (type) {
            case ASTNode::Type::None:              return "None";

            // statements
            case ASTNode::Type::Call:              return "Call";
            case ASTNode::Type::While:             return "While";
            case ASTNode::Type::For:               return "For";
            case ASTNode::Type::If:                return "If";
            case ASTNode::Type::Else:              return "Else";
            case ASTNode::Type::Return:            return "Return";
            case ASTNode::Type::Continue:          return "Continue";
            case ASTNode::Type::Break:             return "Break";
            case ASTNode::Type::Block:             return "Block";
            case ASTNode::Type::Declaration:       return "Declaration";
            case ASTNode::Type::Assignment:        return "Assignment";
            case ASTNode::Type::Expression:        return "Expression";

            // pratt / expression system
            case ASTNode::Type::Value:             return "Value";
            case ASTNode::Type::Prefix:            return "Prefix";
            case ASTNode::Type::Infix:             return "Infix";
            case ASTNode::Type::Postfix:           return "Postfix";
            case ASTNode::Type::Ternary:           return "Ternary";
            case ASTNode::Type::OpeningWrapper:    return "OpeningWrapper";
            case ASTNode::Type::ClosingWrapper:    return "ClosingWrapper";
            case ASTNode::Type::ExprEnd:           return "ExprEnd";
            case ASTNode::Type::TernarySeparator:  return "TernarySeparator";
        }

        return "Unknown";
    }

    static void print_node(const ASTNode* node, int depth, bool is_last) {
        using namespace ansiColors;

        if (!node) return;

        // indentation
        for (int i = 0; i < depth - 1; i++) {
            std::cout << "|   ";
        }

        if (depth > 0) {
            std::cout << (is_last ? "'- " : "|-- ");
        }

        // node header
        std::cout
            << bright_magenta << "[" << type_name(node->type) << "] " << reset
            << bright_yellow  << node->token.id << reset << " "
            << bright_green   << node->token.label << reset << " "
            << bright_white   << node->token.data << reset
            << "\n";

        // children
        const auto& kids = node->children;
        for (size_t i = 0; i < kids.size(); i++) {
            print_node(kids[i].get(), depth + 1, i + 1 == kids.size());
        }
    }
};
