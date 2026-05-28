#include "../includes/utils.hpp"
#include "../includes/combined_include.hpp"
#include "../includes/ansi_colors.hpp"
#include "../includes/json.hpp"

using json = nlohmann::json;

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
        string color = "\033[0;33m";   // orange-ish
        if (fatal) color = "\033[1;31m";    // red

        cerr << color << "Error";
    }

    if (!filename.empty()) {
        cerr << ansiColors::reset
                  << " in file '" << ansiColors::yellow << filename << "'";
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


static string join_path(const string& a, const string& b) {
    return a.empty() ? b : a + "." + b;
}

string find_pattern_in_json(string& pattern, json& j) {
    for (auto& [key, value] : j.items()) {
        smatch m;
        if (regex_match(key, m, regex(pattern))) {
            return key;
        }
    }
}

void JsonValidator::validate(const json& j, const Schema& schema) {

    if (!schema.root_name.empty() && !j.contains(schema.root_name)) {
        utils::error("Missing root: " + schema.root_name);
    }

    const json& root = schema.root_name.empty() ? j : j.at(schema.root_name);

    for (const auto& item : (root.is_array() ? root : json::array({root}))) {
        validate_node(item, schema.root, schema.root_name);
    }
}


void JsonValidator::validate_node(
    const json& node,
    const ObjectSchema& schema,
    const string& path
) {
    const string current_path =
        path.empty() ? schema.name : path + "." + schema.name;

    if (schema.type != Type::Object) {

        switch (schema.type) {
            case Type::String:
                if (!node.is_string())
                    utils::error("Type mismatch (expected string): " + current_path);
                break;

            case Type::Int:
                if (!node.is_number_integer())
                    utils::error("Type mismatch (expected int): " + current_path);
                break;

            case Type::Bool:
                if (!node.is_boolean())
                    utils::error("Type mismatch (expected bool): " + current_path);
                break;

            case Type::Array:
                if (!node.is_array())
                    utils::error("Type mismatch (expected array): " + current_path);
                break;

            default:
                break;
        }

        return;
    }

    if (!node.is_object()) {
        utils::error("Type mismatch (expected object): " + current_path);
    }

    for (const auto& child : schema.fields) {

        string child_path = current_path + "." + child.name;

        if (!node.contains(child.name)) {
            if (child.optional) continue;
            utils::error("Missing field: " + child_path);
        }

        validate_node(node.at(child.name), child, current_path);
    }
}

}