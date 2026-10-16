// lexer.cpp

#include "lexer.hpp"
#include "json_validator.hpp"
#include "utils.hpp"
#include "compiler_cxt.hpp"
#include "ansi_colors.hpp"
#include "error_info.hpp"

Lexer::Lexer(
    CompilerCxt& cxt,
    std::filesystem::path file,
    std::vector<Rule> Rules
) : cxt(cxt),
    rules(std::move(Rules)),
    json_validator(cxt, json_schema)
{
    json lex_data = load_and_validate_json(cxt, file, json_validator);

    auto& arr = lex_data.at("tokens");
    rules.reserve(rules.size() + arr.size());

    for (auto& [key, val] : arr.items()) {
        rules.emplace_back(
            std::stoi(key),
            val[0].get<std::string>(),
            val[1].get<std::string>(),
            val[2].get<bool>()
        );
    }
}

std::vector<Token> Lexer::run(const std::string& input) {
    std::vector<Token> output;
    output.reserve(input.size() / 2);

    size_t pos = 0;
    const size_t input_size = input.size();

    ErrorLocation loc;
    loc.file = cxt.current_file;

    int total_lines = std::count(input.begin(), input.end(), '\n') + 1;
    int newlines_left = total_lines;

    while (pos < input_size) {
        int old_newlines_left = newlines_left;

        std::string_view remaining(input.data() + pos, input_size - pos);

        newlines_left = std::count(remaining.begin(), remaining.end(), '\n');
        loc.line = total_lines - newlines_left;

        if (newlines_left != old_newlines_left) {
            loc.column = 0;
        }

        size_t best_len = 0;
        const Rule* best_rule = nullptr;
        std::string best_match;

        for (const auto& rule : rules) {

            const char* begin = remaining.data();
            const char* end   = begin + remaining.size();

            std::cmatch m;

            if (std::regex_search(begin, end, m, rule.pattern)) {

                std::string match = m.str();
                size_t len = match.size();

                if (len > best_len ||
                    (len == best_len && best_rule && rule.id < best_rule->id)) {

                    best_len = len;
                    best_rule = &rule;
                    best_match = match;
                }
            }
        }

        if (!best_rule) {
            utils::error(
                "invalid or unrecognized token.",
                cxt,
                &loc,
                "",
                false,
                false
            );
            break;
        }

        if (!best_rule->skip) {
            output.emplace_back(Token{
                best_rule->id,
                best_rule->label,
                best_match,
                loc,
                best_rule->skip
            });
        }

        pos += best_len;
        loc.column += best_len;
    }

    return output;
}

Lexer::Rule* Lexer::find_lex_rule(std::string key) {
    for (auto& lex : rules) {
        if (lex.label == key || std::to_string(lex.id) == key)
            return &lex;
    }
    return nullptr;
}

void Lexer::print_output(const std::vector<Token>& output) const {
    using namespace ansiColors;

    // Group tokens by file.
    std::map<std::string, std::vector<const Token*>> grouped;

    for (const auto& t : output) {
        std::string file = t.loc.file
            ? t.loc.file->string()
            : "<unknown>";

        grouped[file].push_back(&t);
    }

    std::cout << bold << cyan
              << "\n========== LEXER OUTPUT ==========\n"
              << reset;

    for (const auto& [file, tokens] : grouped) {
        std::cout << "\n"
                  << bold << bright_cyan
                  << "File: " << file << "\n"
                  << reset;

        std::cout << bold
                  << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(14) << "LABEL"
                  << std::setw(10) << "LINE"
                  << std::setw(10) << "COLUMN"
                  << "DATA\n"
                  << reset;

        std::cout << "-----------------------------------------------\n";

        for (const Token* t : tokens) {
            std::cout
                << bright_yellow << std::left << std::setw(6)
                << t->id << reset

                << bright_green << std::left << std::setw(14)
                << t->label << reset

                << bright_blue << std::left << std::setw(10)
                << t->loc.line << reset

                << orange << std::left << std::setw(10)
                << t->loc.column << reset

                << magenta << utils::visualize_whitespaces(t->data)
                << "\n";
        }
    }

    std::cout << "\n" << bold << cyan
              << "==================================\n"
              << reset;
}