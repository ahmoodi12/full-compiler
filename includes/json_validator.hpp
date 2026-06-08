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
        std::string name;
        Type type = Type::Object;
        std::vector<Schema> fields;
        bool optional = false;
        bool is_tuple = false;

        const char* type_name() const;

        Schema() = default;

        Schema(
            std::string name,
            Type type,
            std::vector<Schema> fields = {},
            bool optional = false,
            bool is_tuple = false
        )
            : name(std::move(name)),
            type(type),
            fields(std::move(fields)),
            optional(optional),
            is_tuple(is_tuple) {}

        Schema(std::string name) : name(std::move(name)) {}
    };
    
    CompilerCxt& cxt;
    const Schema schema;

    JsonValidator(CompilerCxt& cxt, const Schema schema)
        : schema(schema), cxt(cxt) {}

    bool validate(const json& j);

private:
    std::vector<std::string> find_patterns_in_json(
        const std::string& pattern,
        const json& j
    );

    bool validate_node(
        const json& node,
        const Schema& schema,
        const std::string& path,
        int depth,
        std::vector<std::string>& error_path,
        bool report_error = 1
    );

    bool validate_children(
        const json& node,
        const Schema& schema,
        const std::string& path,
        int depth,
        std::vector<std::string>& error_path,
        bool report_error
    );
};

json load_and_validate_json(
    CompilerCxt& cxt,
    const std::string& filename,
    JsonValidator& validator
);