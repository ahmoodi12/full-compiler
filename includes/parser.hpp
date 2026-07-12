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
                                    // token OR capture name: token
                                    {".+", T::String, {}, true},
                                    {".+", T::Object, {
                                        {".+", T::String, {}, true},
                                    }, true}
                                }}
                            }, true},
                            
                            {".+", T::Array, {
                                // token OR capture name: token
                                {".+", T::String, {}, true},
                                {".+", T::Object, {
                                    {".+", T::String, {}, true},
                                }, true}
                            }, true}
                        }}
                    }}
                }}
            }}
        }
    };

public:
    struct Rule {
        std::string statement;
        std::vector<RuleBase> pattern;
        int parent_i = -1;   // index in grammar rules

        std::string stringify_pattern() {
            std::string out;
            out += "[";
            for (auto& token : pattern) {
                out += token.label + ", ";
            }
            out += "]";
            return out;
        }
    };

    struct StmtMatch {
        bool valid = false;
        size_t size = 0;
        std::vector<ASTNode> exprs; 
        std::vector<ASTNode> sub_stmts;
        std::unordered_map<std::string, Token*> captures;
        
        PrattParser::ParseError error;

        Rule* rule;
    };
        
    PrattParser pratt_parser;
    JsonValidator json_validator;

    Lexer& lexer;

    std::vector<Rule> grammar_rules;

    std::unordered_map<std::string, std::string> capture_tokens; // label: capture

    std::unordered_map<std::string, Rule*> by_statement;
    
    std::vector<Rule> variable_sub_statements;

    int64_t pos = 0;
    std::vector<Token>* tokens = nullptr;

    CompilerCxt& cxt;

    void add_seq_tokens(json &sequence, Parser::Rule &rule);

    void parse_grammar_rule(json &pattern, const std::string &statement_str, std::vector<Parser::Rule> &rules, bool allow_optionals, int seq_i);

    void parse_grammar_rules(json &grammar, std::vector<Parser::Rule> &rules, bool is_grammar_rules);

    Parser(
        CompilerCxt &cxt,
        const std::string &filename,
        Lexer &lexer,
        std::vector<PrattParser::Rule> pratt_rules = {});

    Parser::StmtMatch parse_statements(std::vector<ASTNode> &output, bool emit_errors = 1, std::function<bool(const Token&)> stop = [](const Token& t){return false;});

    Parser::StmtMatch match_stmt(Rule &rule);

    ASTNode parse_stmt(StmtMatch *match);

    std::vector<ASTNode> run(std::vector<Token> *input);
};

