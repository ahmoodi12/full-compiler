#pragma once

#include "utils.hpp"
#include "combined_include.hpp"
#include "json.hpp"
#include "compiler_cxt.hpp"

using json = nlohmann::json;


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
