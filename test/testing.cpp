
#include "../includes/combined_include.hpp"
#include "../includes/json_validator.hpp"

/*
cd builds/windows/test
ninja
*/

int main() {
    CompilerCxt cxt;

    cxt.current_file = utils::get_file_path("C:/projects/full compiler/data/c frontend data/lex_data.json");

    JsonValidator validator(cxt, {
        "",
        JsonValidator::Type::Object,
        {
            {
                "regexes",
                JsonValidator::Type::Object,
                {
                    {
                        "\\d+",
                        JsonValidator::Type::Int
                    }
                }
            }
        }
    });

    validator.validate(json::parse(utils::read_file(cxt.current_file)));

    return 0;
}