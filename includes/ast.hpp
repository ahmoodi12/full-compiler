#pragma once

#include "token.hpp"
#include "utils.hpp"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include "ansi_colors.hpp"
#include <algorithm>

struct ASTNode {
    Token token;

    std::unordered_map<std::string, ASTNode*> captures;
    std::vector<std::unique_ptr<ASTNode>> owned_captures;

    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode() = default;

    explicit ASTNode(Token tok)
        : token(std::move(tok)) {}

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    ASTNode(ASTNode&&) noexcept = default;
    ASTNode& operator=(ASTNode&&) noexcept = default;
};

inline bool valid_ast(const ASTNode& node) {
    return node.token.id != -1 || !node.token.label.empty();
}

inline void add_child(ASTNode& node, std::unique_ptr<ASTNode>& child) {
    node.children.push_back(std::move(child));
}

inline void add_child(ASTNode& node, ASTNode& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}

inline void add_child(ASTNode& node, ASTNode&& child) {
    node.children.push_back(std::make_unique<ASTNode>(std::move(child)));
}


class ASTPrinter {
public:
    static void print(const std::vector<ASTNode>* root) {
        using namespace ansiColors;

        std::cout << bold << cyan
                  << "\n===== AST =====\n"
                  << reset;

        if (!root) {
            std::cout << red << "empty AST\n" << reset;
            return;
        }

        for (auto& node : *root){
            print_node(&node, "", true);
            std::cout << "\n";
        }

        std::cout << bold << cyan
                  << "===============\n"
                  << reset;
    }

private:
    /*
        Example output:

        1 Program
        |-- 2 Function
        |   |-- @name = 3 Identifier foo
        |   '-- 4 Body
        |       |-- 5 ...
        |       '-- 6 ...
        |
        '-- 7 Function
    */
    static void print_node(
        const ASTNode* node,
        const std::string& prefix,
        bool last
    ) {
        using namespace ansiColors;

        if (!node)
            return;

        // Node itself
        if (!prefix.empty()) {
            std::cout << prefix
                      << (last ? "'- " : "|-- ");
        }

        std::cout
            << orange << node->token.id << reset
            << " "
            << bright_green << node->token.label << reset;

        if (!node->token.data.empty()) {
            std::cout << " "
                      << bright_white << node->token.data << reset;
        }

        std::cout << '\n';


        /*
            Captures and children are treated as one logical list.

            This is important because a capture may be followed by a
            normal child, and the tree connector needs to know whether
            there is anything after it.
        */

        const size_t capture_count = node->captures.size();
        const size_t child_count   = node->children.size();
        const size_t total         = capture_count + child_count;

        if (total == 0)
            return;


        /*
            unordered_map has no stable ordering.

            If deterministic output is desirable, copy the captures into
            a vector and sort them by name.
        */
        std::vector<std::pair<std::string, ASTNode*>> captures;
        captures.reserve(capture_count);

        for (const auto& [name, capture] : node->captures) {
            captures.emplace_back(name, capture);
        }

        std::sort(
            captures.begin(),
            captures.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            }
        );


        for (size_t i = 0; i < total; ++i) {
            const bool is_last = (i + 1 == total);

            const std::string child_prefix =
                prefix + (last ? "    " : "|   ");

            if (i < capture_count) {
                // Capture
                const auto& [name, capture] = captures[i];

                std::cout << child_prefix
                          << (is_last ? "'- " : "|-- ")
                          << bright_magenta << '@' << reset
                          << bright_blue << name << reset
                          << " = ";

                print_capture(capture, child_prefix, is_last);
            }
            else {
                // Normal child
                const size_t child_index = i - capture_count;

                print_node(
                    node->children[child_index].get(),
                    child_prefix,
                    is_last
                );
            }
        }
    }


    static void print_capture(
        const ASTNode* node,
        const std::string& prefix,
        bool last
    ) {
        using namespace ansiColors;

        if (!node) {
            std::cout << red << "null" << reset << '\n';
            return;
        }

        /*
            A capture has already printed:

                |-- @name =

            Therefore print only the captured node itself here,
            without another tree connector.
        */
        std::cout
            << orange << node->token.id << reset
            << " "
            << bright_green << node->token.label << reset;

        if (!node->token.data.empty()) {
            std::cout << " "
                      << bright_white << node->token.data << reset;
        }

        std::cout << '\n';

        /*
            If the captured node itself has children/captures, continue
            printing them underneath it.
        */
        const size_t capture_count = node->captures.size();
        const size_t child_count   = node->children.size();
        const size_t total         = capture_count + child_count;

        if (total == 0)
            return;

        std::vector<std::pair<std::string, ASTNode*>> captures;
        captures.reserve(capture_count);

        for (const auto& [name, capture] : node->captures) {
            captures.emplace_back(name, capture);
        }

        std::sort(
            captures.begin(),
            captures.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            }
        );

        for (size_t i = 0; i < total; ++i) {
            const bool is_last = (i + 1 == total);

            const std::string child_prefix =
                prefix + (last ? "    " : "|   ");

            if (i < capture_count) {
                const auto& [name, capture] = captures[i];

                std::cout << child_prefix
                          << (is_last ? "'- " : "|-- ")
                          << bright_magenta << '@' << reset
                          << bright_blue << name << reset
                          << " = ";

                print_capture(capture, child_prefix, is_last);
            }
            else {
                const size_t child_index = i - capture_count;

                print_node(
                    node->children[child_index].get(),
                    child_prefix,
                    is_last
                );
            }
        }
    }
};