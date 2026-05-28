#pragma once

#include "combined_include.hpp"

namespace utils {

void error(
    std::string prompt,
    std::string filename = "",
    bool show_warnings = false,
    bool is_warning = false,
    bool fatal = true
);

filesystem::path get_file_path(std::string filename);
std::string read_file(const filesystem::path& path);

class JsonValidator {
public:
    enum class Type {
        String,
        Int,
        Bool,
        Array,
        Object
    };

    struct ObjectSchema {
        std::string name;
        Type type;

        std::vector<ObjectSchema> fields; // recursive children
        bool optional = false;
    };

    struct Schema {
        std::string root_name; // empty = whole JSON is root
        ObjectSchema root;
    };

    static void validate(const json& j, const Schema& schema);

private:
    static void validate_node(
        const json& node,
        const ObjectSchema& schema,
        const std::string& path
    );
};

}