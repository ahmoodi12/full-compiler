#pragma once

#include "combined_include.hpp"
#include "json.hpp"

class CompilerCxt;
using json = nlohmann::json;


class JsonValidator {
public:
    // TODO: add float handling
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
        bool is_tuple = false;
    };
    
    CompilerCxt& cxt;
    const Schema schema;
    JsonValidator(CompilerCxt& cxt, const Schema schema) : schema(schema), cxt(cxt) {}

    bool validate(const json& j);

private:
    vector<string> find_patterns_in_json(
        const string& pattern,
        const json& j
    );

    bool check_type(
        const json& node,
        const Schema& schema,
        const string& path,
        vector<string>& error_path
    );

    bool validate_node(
        const json& node,
        const Schema& schema,
        const string& path,
        int depth,
        vector<string>& error_path
    );

    bool validate_children(
        const json& node,
        const Schema& schema,
        const string& path,
        int depth,
        vector<string>& error_path
    );
};

json load_and_validate_json(CompilerCxt& cxt, const string& filename, JsonValidator& validator);