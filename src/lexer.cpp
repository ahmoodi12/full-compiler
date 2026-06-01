// lexer.cpp

#include "lexer.hpp"
#include "json_validator.hpp"
#include "utils.hpp"
#include "compiler_cxt.hpp"
#include "ansi_colors.hpp"

Lexer::Lexer(
    CompilerCxt& cxt,
    string filename,
    vector<TokenRule> Rules,
    unordered_map<uint32_t, string> names
) : cxt(cxt),
    rules(Rules),
    debug_names(names),
    json_validator(cxt, json_schema)
{   
    if (!filename.empty()) {
        json lex_data = load_and_validate_json(cxt, filename, json_validator);

        for (auto& [key, val] : lex_data.at("regexes").items()) {
            rules.emplace_back(stoi(key), val[0], val[1]);
        }

        if (lex_data.contains("debug_names")) {
            for (auto& [key, val] : lex_data.at("debug_names").items()) {
                debug_names[stoi(key)] = val.get<string>();
            }
        }
    }
}

vector<Lexer::Token> Lexer::run(const string& input) {

    last_output.clear();

    uint64_t pos = 0;

    vector<Lexer::Token> temp_holder;

    while (pos < input.size()) {
        string remaining = input.substr(pos);
        temp_holder.clear();

        for (auto& rule : rules) {
            smatch m;

            if (regex_search(remaining, m, rule.pattern)) {
                string matched_str = m.str();

                temp_holder.emplace_back(
                    Lexer::Token {
                        rule.id,
                        debug_names[rule.id],
                        matched_str,
                        rule.skip
                });
            }
        }
        if (temp_holder.empty()) {
            utils::error("wasn't able to lex from position " + to_string(pos) + ", invalid char or unadded Lexer::token type.", 
            cxt, false, false);
        } else {
            int longest = 0;
            Lexer::Token* longest_token = nullptr;
            for (auto& match : temp_holder) {
                int length = match.data.length();
                if (length > longest || 
                   (length == longest && longest_token 
                    && (longest_token->id > match.id))) {
                    longest = length;
                    longest_token = &match;
                }
            }

            if (!longest_token->skip) last_output.push_back(*longest_token);
            pos += longest;
        }
    }
    return last_output;
}


void Lexer::print_last_output() const {
    using namespace ansiColors;

    cout << bold << cyan
              << "\n===== LEXER OUTPUT =====\n"
              << reset;

    for (const auto& t : last_output) {
        cout
            << bright_yellow << setw(4) << t.id << reset << "  "
            << bright_green  << setw(12) << t.name << reset << "  "
            << bright_white  << utils::visualize_whitespaces(t.data) << reset
            << "\n";
    }

    cout << bold << cyan
              << "========================\n"
              << reset;
}