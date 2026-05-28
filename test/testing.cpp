
#include "../includes/combined_include.hpp"
#include "../includes/utils.hpp"

/*
cd builds/windows/test
ninja
*/

int main() {

    utils::JsonValidator validator ({
        "",
        utils::JsonValidator::Type::Object,
        {{
            "regexes",
            utils::JsonValidator::Type::Object,
            {{                          // double brackets, one for the vector init one for the objectschema init
                "\\d+",
                utils::JsonValidator::Type::Int
            }}
        }}
    });

    validator.validate(json::parse(utils::read_file(utils::get_file_path("C:/projects/full compiler/data/c frontend data/lex_data.json"))));

    return 0;
}