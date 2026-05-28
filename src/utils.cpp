// utils.cpp

#include "../includes/utils.hpp"

namespace utils {

void error(
    string prompt,
    string filename,
    bool show_warnings,
    bool is_warning,
    bool fatal
) {
    if (!show_warnings && is_warning) return;

    if (is_warning) {
        cerr << ansiColors::yellow << "Warning";
    } else {
        string color = "\033[0;33m";

        if (fatal) color = "\033[1;31m";

        cerr << color << "Error";
    }

    if (!filename.empty()) {
        cerr << ansiColors::reset
             << " in file '" << ansiColors::yellow
             << filename << "'";
    }

    cerr << ":\n";

    cerr << ansiColors::bright_blue
         << ">> " << prompt << " <<\n\n"
         << ansiColors::reset;

    if (!is_warning && fatal) {
        exit(1);
    }
}

filesystem::path get_file_path(string filename) {
    filesystem::path file_path(filename);

    if (file_path.is_relative()) {
        file_path = filesystem::current_path() / file_path;
    }

    if (!filesystem::exists(file_path)) {
        utils::error("File does not exist: " + filename);
    }

    return file_path;
}

string read_file(const filesystem::path& path) {
    ifstream file(path, ios::binary);

    if (!file) {
        utils::error("Failed to open file: " + path.string());
    }

    return string(
        istreambuf_iterator<char>(file),
        istreambuf_iterator<char>()
    );
}

vector<string> JsonValidator::find_patterns_in_json(
    const string& pattern,
    const json& j
) {
    vector<string> matches;

    regex r(pattern);

    for (auto& [key, value] : j.items()) {
        if (regex_match(key, r)) {
            matches.push_back(key);
        }
    }

    return matches;
}


bool JsonValidator::validate_children(    
    const json& node,
    const JsonValidator::Schema& schema,
    const string& path
    ) {
    for (const auto& child : schema.fields) {

        vector<string> matches =
            find_patterns_in_json(child.name, node);

        if (matches.empty()) {

            if (child.optional) {
                continue;
            }

            utils::error(
                "Missing field (pattern): " + child.name +
                " at " + path
            );

            return 0;
        }

        for (const auto& matched_key : matches) {

            if (!validate_node(
                node.at(matched_key),
                child,
                path + "." + matched_key
            )) {
                return 0;
            }
        }
    }
    return 1;
}


bool JsonValidator::validate(const json& j) {
    vector<string> root_matches;

    if (!schema.name.empty()) {
        root_matches = find_patterns_in_json(schema.name, j);

        if (root_matches.empty()) {
            utils::error("Missing root key: " + schema.name);
            return 0;
        }
    } else {
        for (auto& [key, val] : j.items()) {
            root_matches.push_back(key);
        }
    }

    for (const auto& key : root_matches) {
        if (!validate_node(j.at(key), schema, key)) {
            return 0;
        }
    }

    return 1;
}


bool JsonValidator::validate_node(
    const json& node,
    const JsonValidator::Schema& schema,
    const string& path
) {
    const string current_path = path;

    switch (schema.type) {

        case Type::String:
            if (!node.is_string()) {
                utils::error(
                    "Type mismatch (expected string): " + current_path
                );
                return 0;
            }
            return 1;

        case Type::Int:
            if (!node.is_number_integer()) {
                utils::error(
                    "Type mismatch (expected int): " + current_path
                );
                return 0;
            }
            return 1;

        case Type::Bool:
            if (!node.is_boolean()) {
                utils::error(
                    "Type mismatch (expected bool): " + current_path
                );
                return 0;
            }
            return 1;

        case Type::Array:
            if (!node.is_array()) {
                utils::error(
                    "Type mismatch (expected array): " + current_path
                );
                return 0;
            }

            for (size_t i = 0; i < node.size(); i++) {

                for (const auto& child : schema.fields) {

                    if (!validate_node(
                        node[i],
                        child,
                        current_path + "[" + to_string(i) + "]"
                    )) {
                        return 0;
                    }
                }
            }

            return 1;

        case Type::Object:
            break;
    }

    if (!node.is_object()) {
        utils::error(
            "Type mismatch (expected object): " + current_path
        );
        return 0;
    }

    return validate_children(node, schema, current_path);
}

}