
#include "includes/combined_include.hpp"
#include "includes/utils.hpp"


int main() {
    utils::JsonValidator::Schema schema {
    "",
    {
        "regexes",
        utils::JsonValidator::Type::Object,
        {
            "id",
            utils::JsonValidator::Type::,

        }
    }
    }

    return 0;
}