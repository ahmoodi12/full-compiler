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
                {"(statement rules|variables)", T::Object, {
                    {".+", T::Object, {
                        {"pattern", T::Array, {
                            {".+", T::Object, {
                                {"optional", T::Array, {
                                    {".+", T::String, {}, true}
                                }}
                            }, true},
                            
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
    struct TokenRule : RuleBase {
        bool commit_point = false;
    };

    struct Sequence {
        bool optional = false;
        std::vector<TokenRule> sequence;
    };

    struct Rule {
        std::string statement;
        std::vector<Sequence> pattern;

        std::string stringify_pattern() {
            std::string out = "[";
            for (auto& item : pattern) {
                out += item.optional ? "{\"optional\": [" : "[";

                for (auto& token : item.sequence) {
                    out += token.label + ", ";
                }
                
                out += item.optional ? "]}, " : "], ";
            }
            out += "]";
            return out;
        }
    };

    struct StmtMatch {
        bool valid = false;
        int size = 0;
        std::vector<ASTNode> exprs; // ONLY expr results
        std::vector<ASTNode> sub_stmts;
        Rule* rule;
    };
        
    PrattParser pratt_parser;
    JsonValidator json_validator;

    Lexer& lexer;

    std::vector<Rule> grammar_rules;

    std::unordered_map<std::string, Rule*> by_statement;
    
    std::vector<Rule> variable_sub_statements;

    size_t pos = 0;
    std::vector<Token>* tokens = nullptr;

    CompilerCxt& cxt;

    void add_seq_tokens(json sequence, Sequence &item);

    void parse_grammar_rule(json &pattern, Parser::Rule &rule, std::string &statement_str);

    void parse_grammar_rules(json &grammar, std::vector<Parser::Rule> &rules, bool add_to_by_statement);

    bool token_is_unique(Rule stmt, TokenRule token, int tok_i);

    int parse_commit_points(Rule stmt, int tok_i, Sequence seq);

    Parser(
        CompilerCxt &cxt,
        const std::string &filename,
        Lexer &lexer,
        std::vector<PrattParser::Rule> pratt_rules = {});

    void parse_statements(std::vector<ASTNode> &output);

    StmtMatch match_stmt(Rule rule, bool committed = 0);

    ASTNode parse_stmt(Rule *rule, StmtMatch *match);

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
