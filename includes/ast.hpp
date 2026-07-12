#pragma once

#include "token.hpp"
#include "utils.hpp"
#include <memory>
#include <vector>
#include "ansi_colors.hpp"

struct ASTNode {
    // main token, acts as a label
    Token token;

    std::unordered_map<std::string, Token*> captures;

    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode() = default;
    explicit ASTNode(Token tok)
        : token(std::move(tok)) {}
};

inline bool valid_ast(const ASTNode& node) {
    return node.token.id != -1 && node.token.label.empty();
}

inline void add_child(ASTNode& node, ASTNode& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}
inline void add_child(ASTNode& node, ASTNode&& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}

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

        // print captures
        size_t capture_count = node->captures.size();
        size_t child_count = node->children.size();

        size_t i = 0;
        for (const auto& [name, tok] : node->captures) {
            // indentation
            for (int j = 0; j < depth; ++j)
                std::cout << "|   ";

            // draw tree branch
            bool last_capture = (++i == capture_count && child_count == 0);
            std::cout << (last_capture ? "'- " : "|-- ");

            std::cout
                << bright_magenta << '@' << reset
                << bright_blue << name << reset
                << " = ";

            if (tok) {
                std::cout
                    << bright_yellow << tok->id << reset << " "
                    << bright_green  << tok->label << reset << " "
                    << bright_white  << tok->data << reset;
            } else {
                std::cout << "<null>";
            }

            std::cout << '\n';
        }

        // blank line between captures and children
        if (!node->captures.empty() && !node->children.empty()) {
            for (int j = 0; j < depth; ++j)
                std::cout << "|   ";
            std::cout << '\n';
        }

        // children
        const auto& kids = node->children;
        for (size_t i = 0; i < kids.size(); ++i) {
            print_node(kids[i].get(), depth + 1, i + 1 == kids.size());
        }
    }
};
