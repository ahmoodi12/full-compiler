#include "../includes/json_validator.hpp"

using namespace std;

namespace {

// split "a.b[0].c" → ["a", "b", "[0]", "c"]
vector<string> split_path(const string& path) {
    vector<string> parts;
    string cur;

    for (char c : path) {
        if (c == '.') {
            if (!cur.empty()) parts.push_back(cur);
            cur.clear();
        }
        else if (c == '[') {
            if (!cur.empty()) {
                parts.push_back(cur);
                cur.clear();
            }
            cur += c;
        }
        else if (c == ']') {
            cur += c;
            parts.push_back(cur);
            cur.clear();
        }
        else {
            cur += c;
        }
    }

    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

// keep only the failing branch inside JSON
json keep_only_path(json node, const vector<string>& path, size_t idx = 0) {
    if (idx >= path.size()) return node;

    const string& part = path[idx];

    // array index like [2]
    if (!part.empty() && part.front() == '[') {
        size_t i = stoi(part.substr(1, part.size() - 2));

        json arr = json::array();

        if (node.is_array() && i < node.size()) {
            arr.push_back(keep_only_path(node[i], path, idx + 1));
        }

        return arr;
    }

    // object key
    if (node.is_object()) {
        json obj;

        if (node.contains(part)) {
            obj[part] = keep_only_path(node.at(part), path, idx + 1);
        }

        return obj;
    }

    return node;
}

} // namespace


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
    const string& path,
    int depth,
    vector<string>& error_path
) {
    bool valid = true;

    for (const auto& child : schema.fields) {

        vector<string> matches = find_patterns_in_json(child.name, node);

        if (matches.empty()) {

            if (child.optional) continue;

            if (error_path.empty())
                error_path.push_back(path);

            utils::error(
                "Missing field (pattern): " + child.name,
                cxt.current_file.string(),
                cxt.show_warnings,
                false,
                false
            );

            valid = false;
            continue;
        }

        for (const auto& key : matches) {

            string new_path = path.empty() ? key : path + "." + key;

            if (!validate_node(
                node.at(key),
                child,
                new_path,
                depth + 1,
                error_path
            )) {
                valid = false;
            }
        }
    }

    return valid;
}


bool JsonValidator::validate(const json& j) {
    bool valid = true;
    vector<string> error_path;

    if (!schema.name.empty()) {

        auto root_matches = find_patterns_in_json(schema.name, j);

        if (root_matches.empty()) {

            utils::error(
                "Missing root key: " + schema.name,
                cxt.current_file.string(),
                cxt.show_warnings,
                false,
                false
            );

            return false;
        }

        for (const auto& key : root_matches) {

            if (!validate_node(
                j.at(key),
                schema,
                key,
                0,
                error_path
            )) {
                valid = false;
            }
        }
    }
    else {
        if (!validate_node(j, schema, "", 0, error_path)) {
            valid = false;
        }
    }

    // 🔥 PRINT FAILING SUBTREE
    if (!error_path.empty()) {

        vector<string> parts = split_path(error_path[0]);

        json subtree = keep_only_path(j, parts);

        cout << "\n--- FAILED SUBTREE ---\n";
        cout << subtree.dump(4) << endl;
    }

    return valid;
}


bool JsonValidator::validate_node(
    const json& node,
    const JsonValidator::Schema& schema,
    const string& path,
    int depth,
    vector<string>& error_path
) {
    switch (schema.type) {

        case Type::String:
            if (!node.is_string()) {

                if (error_path.empty())
                    error_path.push_back(path);

                utils::error(
                    "Type mismatch (expected string): " + path,
                    cxt.current_file.string(),
                    cxt.show_warnings,
                    false,
                    false
                );

                return false;
            }
            return true;

        case Type::Int:
            if (!node.is_number_integer()) {

                if (error_path.empty())
                    error_path.push_back(path);

                utils::error(
                    "Type mismatch (expected int): " + path,
                    cxt.current_file.string(),
                    cxt.show_warnings,
                    false,
                    false
                );

                return false;
            }
            return true;

        case Type::Bool:
            if (!node.is_boolean()) {

                if (error_path.empty())
                    error_path.push_back(path);

                utils::error(
                    "Type mismatch (expected bool): " + path,
                    cxt.current_file.string(),
                    cxt.show_warnings,
                    false,
                    false
                );

                return false;
            }
            return true;

        case Type::Array:
            if (!node.is_array()) {

                if (error_path.empty())
                    error_path.push_back(path);

                utils::error(
                    "Type mismatch (expected array): " + path,
                    cxt.current_file.string(),
                    cxt.show_warnings,
                    false,
                    false
                );

                return false;
            }

            for (size_t i = 0; i < node.size(); i++) {
                for (const auto& child : schema.fields) {

                    string new_path = path + "[" + to_string(i) + "]";

                    if (!validate_node(
                        node[i],
                        child,
                        new_path,
                        depth + 1,
                        error_path
                    )) {
                        return false;
                    }
                }
            }

            return true;

        case Type::Object:
            if (!node.is_object()) {

                if (error_path.empty())
                    error_path.push_back(path);

                utils::error(
                    "Type mismatch (expected object): " + path,
                    cxt.current_file.string(),
                    cxt.show_warnings,
                    false,
                    false
                );

                return false;
            }

            return validate_children(node, schema, path, depth, error_path);
    }

    return true;
}