#pragma once

#include "combined_include.hpp"
#include "compiler_cxt.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "json.hpp"
#include "lexer.hpp"

#include <unordered_map>
#include <vector>

// TODO add ternary and postfix operators
class PrattParser {
public:
    enum TypeMask : uint32_t {
        None = 0,
        Value = 1 << 0,
        Prefix = 1 << 1,
        Infix = 1 << 2,
        Postfix = 1 << 3,
        Ternary = 1 << 4,
        OpeningWrapper = 1 << 5,
        ClosingWrapper = 1 << 6,
        ExprEnd = 1 << 7
    };

    struct Rule : RuleBase {
        uint32_t type_mask = 0;
        uint16_t lbp = 0;
        uint16_t rbp = 0;
    };

    CompilerCxt& cxt;

    std::vector<Rule> rules;
    std::unordered_map<uint32_t, Rule*> by_id;
    std::unordered_map<std::string, Rule*> by_label;

    std::vector<Token>* tokens = nullptr;
    size_t pos = 0;

    uint16_t prefix_bp = 100;

    PrattParser(CompilerCxt& cxt, std::vector<Rule> rules);

    void load_json(json& data, const std::vector<Lexer::Rule>& lexer_rules);

    ASTNode parse_expr(uint16_t rbp);
    ASTNode parse_atom();

private:
    Token& peek();
    Token consume();
    bool eof();

    Rule* find_rule(const Token& t);

    static bool has(uint32_t mask, TypeMask t);
};