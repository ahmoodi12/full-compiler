// lexer.cpp

#include "lexer.hpp"

Lexer::Lexer(
    CompilerCxt& cxt,
    string lex_data_file,
    vector<TokenRule> Rules,
    unordered_map<uint32_t, string> names
) : cxt(cxt),
    rules(Rules),
    debug_names(names),
    json_validator(cxt, lexer_json_schema)
{   
    if (!lex_data_file.empty()) {
        filesystem::path lex_data_path = utils::get_file_path(lex_data_file);

        filesystem::path cxt_file_temp = cxt.current_file;
        cxt.current_file = lex_data_path;

        json lex_data = json::parse(utils::read_file(lex_data_path));

        if (!json_validator.validate(lex_data)) exit(1);

        for (auto& [key, val] : lex_data.at("regexes").items()) {
            rules.emplace_back(stoi(key), val[0], val[1]);
        }

        if (lex_data.contains("debug_names")) {
            for (auto& [key, val] : lex_data.at("debug_names").items()) {
                debug_names[stoi(key)] = val.get<string>();
            }
        }

        cxt.current_file = cxt_file_temp;
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
            cxt.program_file.string(), cxt.show_warnings, false);
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

static string escape_ws(const string& s) {
    string out;
    out.reserve(s.size());

    for (unsigned char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;

            // normal space
            case ' ':  out += "-"; break;

            // weird NBSP (your ┬À issue source most likely)
            case 0xC2: case 0xA0:
                out += "⍽";
                break;

            default:
                out += c;
                break;
        }
    }
    return out;
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
            << bright_white  << escape_ws(t.data) << reset
            << "\n";
    }

    cout << bold << cyan
              << "========================\n"
              << reset;
}