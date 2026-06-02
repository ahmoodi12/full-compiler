// lexer.cpp

#include "lexer.hpp"
#include "json_validator.hpp"
#include "utils.hpp"
#include "compiler_cxt.hpp"
#include "ansi_colors.hpp"

Lexer::Lexer(
    CompilerCxt& cxt,
    std::string filename,
    std::vector<Rule> Rules
) : cxt(cxt),
    rules(std::move(Rules)),
    json_validator(cxt, json_schema)
{
    if (!filename.empty()) {
        json lex_data = load_and_validate_json(cxt, filename, json_validator);

        auto& arr = lex_data.at("rules");
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
}

std::vector<Lexer::Token> Lexer::run(const std::string& input) {
    std::vector<Token> output;
    output.reserve(input.size() / 2);

    size_t pos = 0;
    const size_t n = input.size();

    while (pos < n) {

        std::string_view remaining(input.data() + pos, n - pos);

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
                "wasn't able to lex at position " + std::to_string(pos) +
                ", invalid or unrecognized token.",
                cxt,
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
                best_rule->skip
            });
        }

        pos += best_len;
    }

    return output;
}

void Lexer::print_output(std::vector<Token> output) const {
    using namespace ansiColors;

    std::cout << bold << cyan
              << "\n===== LEXER OUTPUT =====\n"
              << reset;

    for (const auto& t : output) {
        std::cout
            << bright_yellow << std::setw(4) << t.id << reset << "  "
            << bright_green  << std::setw(12) << t.label << reset << "  "
            << bright_white  << utils::visualize_whitespaces(t.data) << reset
            << "\n";
    }

    std::cout << bold << cyan
              << "========================\n"
              << reset;
}