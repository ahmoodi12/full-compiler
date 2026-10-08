#pragma once

#include "combined_include.hpp"
#include "utils.hpp"
#include "compiler_cxt.hpp"

class CompilerCxt;

struct JsonPathPart {
    enum class Type {
        Key,
        Index
    };

    Type type = Type::Key;

    std::string key;
    size_t index = 0;

    static JsonPathPart key_part(std::string k) {
        JsonPathPart p;
        p.type = Type::Key;
        p.key = std::move(k);
        return p;
    }

    static JsonPathPart index_part(size_t i) {
        JsonPathPart p;
        p.type = Type::Index;
        p.index = i;
        return p;
    }
};

// TODO: add float handling
enum class JsonType {
    String,
    Int,
    Bool,
    Array,
    Object
};

template <typename Json>
class JsonValidator {
public:
    struct Schema {
        std::string name;
        JsonType type = JsonType::Object;
        std::vector<Schema> fields;
        bool optional = false;
        bool is_tuple = false;

        const char* type_name() const;

        Schema() = default;

        Schema(
            std::string name,
            JsonType type,
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

    bool validate(const Json &j);

private:
    std::vector<std::string> find_patterns_in_json(
        const std::string& pattern,
        const Json& j
    );

    bool validate_node(
        const Json& node,
        const Schema& schema,
        const std::vector<JsonPathPart>& path,
        std::vector<JsonPathPart>& error_path,
        bool report_error = 1
    );

    bool validate_children(
        const Json& node,
        const Schema& schema,
        const std::vector<JsonPathPart>& path,
        std::vector<JsonPathPart>& error_path,
        bool report_error
        );
};


template <typename Json>
Json load_and_validate_json(
    CompilerCxt& cxt,
    const std::string& filename,
    JsonValidator<Json>& validator
) {
    auto file_path = utils::get_file_path(filename, cxt);

    auto old = cxt.current_file;
    cxt.current_file = file_path;

    Json data = Json::parse(utils::read_file(file_path, cxt));

    if (!validator.validate(data))
        std::exit(1);

    cxt.current_file = old;

    return data;
}