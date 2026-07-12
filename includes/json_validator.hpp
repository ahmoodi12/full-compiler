#pragma once

#include "combined_include.hpp"

class CompilerCxt;


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

    struct PathPart {
        enum class Type {
            Key,
            Index
        };

        Type type = Type::Key;

        std::string key;
        size_t index = 0;

        static PathPart key_part(std::string k) {
            PathPart p;
            p.type = Type::Key;
            p.key = std::move(k);
            return p;
        }

        static PathPart index_part(size_t i) {
            PathPart p;
            p.type = Type::Index;
            p.index = i;
            return p;
        }
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

    bool validate(const json &j);

private:
    std::vector<std::string> find_patterns_in_json(
        const std::string& pattern,
        const json& j
    );

    bool validate_node(
        const json& node,
        const Schema& schema,
        const std::vector<PathPart>& path,
        std::vector<PathPart>& error_path,
        bool report_error = 1
    );

    bool validate_children(
        const json& node,
        const Schema& schema,
        const std::vector<PathPart>& path,
        std::vector<PathPart>& error_path,
        bool report_error
    );
};

json load_and_validate_json(
    CompilerCxt& cxt,
    const std::string& filename,
    JsonValidator& validator
);