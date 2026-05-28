// utils.hpp

#pragma once

#include "combined_include.hpp"
#include "json.hpp"
#include "ansi_colors.hpp"

using json = nlohmann::json;

namespace utils {

void error(
    string prompt,
    string filename = "",
    bool show_warnings = false,
    bool is_warning = false,
    bool fatal = true
);

filesystem::path get_file_path(string filename);
string read_file(const filesystem::path& path);

class JsonValidator {
public:
    enum class Type {
        String,
        Int,
        Bool,
        Array,
        Object
    };

    struct Schema {
        string name;
        Type type;
        vector<Schema> fields;
        bool optional = false;
    };

    const Schema& schema;
    
    JsonValidator(const Schema& schema) : schema(schema) {}

    bool validate(const json& j);

private:
    vector<string> find_patterns_in_json(
        const string& pattern,
        const json& j
    );

    bool validate_node(
        const json& node,
        const Schema& schema,
        const string& path
    );

    bool validate_children(
        const json& node,
        const Schema& schema,
        const string& path
    );
};

}