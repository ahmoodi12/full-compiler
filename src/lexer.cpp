// lexer.cpp

#include "../includes/lexer.hpp"

Lexer::Lexer(
    CompilerCxt cxt,
    filesystem::path lex_data_file,
    vector<TokenRule> rules,
    unordered_map<uint32_t, string> names
) : cxt(cxt),
    rules(rules),
    id_names(names)
{
    filesystem::path lex_data_path = utils::get_file_path(lex_data_file.string());

    json lex_data = json::parse(utils::read_file(lex_data_path));
    
    
}

void Lexer::run(const string& input) {

    last_output.clear();

    uint64_t pos = 0;

    bool matched = false;

    while (pos < input.size()) {

        string remaining = input.substr(pos);

        for (auto& rule : rules) {

            smatch m;

            if (matched = regex_match(remaining, m, regex(rule.pattern))) {

                string matched_str = m.str();

                last_output.emplace_back(
                    rule.id,
                    id_names[rule.id],
                    matched_str
        );

                pos += matched_str.size();
                break;
            }
        }

        if (!matched) {
            utils::error("wasn't able to lex from position " + to_string(pos) + ", invalid char or unadded token type.", 
            cxt.program_file.string(), cxt.show_warnings, false);
        }
    }
}