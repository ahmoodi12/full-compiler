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
            {"expr definition", T::Object, {
                {".+", T::Object, {
                    {"([rl]bp|precedence)", T::Int, {}, true},
                    {"associativity", T::String, {}, true},
                    {"types", T::Array, {
                        {"(value|prefix|infix|expr terminator|opening wrapper|closing wrapper|ternary separator|ternary)", T::String}
                    }}
                }}
            }},

            {"grammar", T::Object, {
                {"(statement rules|variables)", T::Object, {
                    {".+", T::Object, {
                        {"pattern", T::Array, {
                            {"", T::Object, {
                                {"optional", T::Array, {
                                    // token OR capture name: token
                                    {".+", T::String, {}, true},
                                    {".+", T::Object, {
                                        {".+", T::String, {}, true},
                                    }, true}
                                }, true},

                                {"(repeat|separator)", T::String, {}, true}
                            }},
                            
                            {"", T::Array, {
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
    struct Rule;

    struct TokenRule : RuleBase {
        std::string capture_name;
        bool repeat = 0;
        std::unique_ptr<TokenRule> separator = nullptr;

        TokenRule copy() {
            return TokenRule{RuleBase{id, label}, capture_name, repeat, separator ? std::make_unique<TokenRule>(separator->copy()) : nullptr};
        }
    };

    struct Rule {
        std::string statement;
        std::vector<TokenRule> pattern;
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
        std::vector<std::unique_ptr<ASTNode>> exprs; 
        std::vector<std::unique_ptr<ASTNode>> sub_stmts;
        std::unordered_map<std::string, ASTNode*> captures;
        std::vector<std::unique_ptr<ASTNode>> owned_captures;
        
        PrattParser::ParseError error;

        std::string statement;

        StmtMatch() = default;

        StmtMatch(const StmtMatch&) = delete;
        StmtMatch& operator=(const StmtMatch&) = delete;

        StmtMatch(StmtMatch&&) noexcept = default;
        StmtMatch& operator=(StmtMatch&&) noexcept = default;
    };
        
    PrattParser pratt_parser;
    JsonValidator json_validator;

    Lexer& lexer;

    std::vector<Rule> grammar_rules;
    
    std::vector<Rule> variable_sub_statements;

    int64_t pos = 0;
    std::vector<Token>* tokens = nullptr;

    CompilerCxt& cxt;

    void add_seq_tokens(json &sequence, Parser::Rule &rule);

    void parse_grammar_rule(json &pattern, const std::string &statement_str, std::vector<Parser::Rule> &rules, int seq_i);

    void parse_grammar_rules(json &grammar, std::vector<Parser::Rule> &rules);

    Parser(
        CompilerCxt &cxt,
        const std::string &filename,
        Lexer &lexer,
        std::vector<PrattParser::Rule> pratt_rules = {});

    Parser::StmtMatch repeat(TokenRule &repeat_token, bool use_seperator, std::function<bool(const Token &)> separator_func);

    Parser::StmtMatch try_all_statements(std::vector<Parser::Rule> &rules, bool error_enabled = 1);

    void match_token(StmtMatch &result, TokenRule &exp_token, Token &token);

    Parser::StmtMatch match_stmt(Rule &rule);

    ASTNode convert_match(StmtMatch *match);

    ASTNode convert_token(Token *token);

    std::vector<ASTNode> run(std::vector<Token> *input);
};
