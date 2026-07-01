#include "json_validator.hpp"
#include "compiler_cxt.hpp"
#include "utils.hpp"

#include <filesystem>
#include <regex>

namespace {

std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string cur;
    parts.reserve(8);

    for (char c : path) {
        if (c == '.') {
            if (!cur.empty()) parts.emplace_back(std::move(cur));
            cur.clear();
        }
        else if (c == '[') {
            if (!cur.empty()) {
                parts.emplace_back(std::move(cur));
                cur.clear();
            }
            cur.push_back(c);
        }
        else if (c == ']') {
            cur.push_back(c);
            parts.emplace_back(std::move(cur));
            cur.clear();
        }
        else {
            cur.push_back(c);
        }
    }

    if (!cur.empty())
        parts.emplace_back(std::move(cur));

    return parts;
}

json keep_only_path(const json& node, const std::vector<std::string>& path, size_t idx = 0) {
    if (idx >= path.size()) return node;

    const auto& part = path[idx];

    if (!part.empty() && part.front() == '[') {
        const size_t i = std::stoul(part.substr(1, part.size() - 2));

        json arr = json::array();

        if (node.is_array() && i < node.size()) {
            arr.push_back(keep_only_path(node[i], path, idx + 1));
        }

        return arr;
    }

    if (node.is_object()) {
        json obj;

        auto it = node.find(part);
        if (it != node.end()) {
            obj[part] = keep_only_path(*it, path, idx + 1);
        }

        return obj;
    }

    return node;
}

std::string format_json_path(std::string path) {
    std::string out = "\"";
    for (char c : path) {
        if (c == '.') {
            out += "\" -> \"";
        } else {
            out += c;
        }
    }
    return out;
}

} // namespace

const char* JsonValidator::Schema::type_name() const {
    switch (type)
    {
    case Type::String:  return "string";
    case Type::Int:     return "int";
    case Type::Bool:    return "bool";
    case Type::Array:   return "array";
    case Type::Object:  return "object";
    }

    return "unknown";
}

std::vector<std::string> JsonValidator::find_patterns_in_json(
    const std::string& pattern,
    const json& j
) {
    static std::unordered_map<std::string, std::regex> cache;

    auto it = cache.find(pattern);

    if (it == cache.end()) {
        it = cache.emplace(pattern, std::regex(pattern)).first;
    }

    const std::regex& r = it->second;

    std::vector<std::string> matches;
    matches.reserve(j.size());

    for (const auto& [key, _] : j.items()) {
        if (std::regex_match(key, r)) {
            matches.emplace_back(key);
        }
    }

    return matches;
}


bool JsonValidator::validate_children(
    const json& node,
    const JsonValidator::Schema& schema,
    const std::string& path,
    int depth,
    std::vector<std::string>& error_path,
    bool report_error
) {
    for (const auto& child : schema.fields) {

        const auto matches = find_patterns_in_json(child.name, node);

        if (matches.empty()) {
            if (child.optional) continue;

            if (error_path.empty())
                error_path.emplace_back(path);

            if (report_error) utils::error(
                "Missing field (pattern): " + child.name,
                cxt,
                "",
                false,
                false
            );

            return false;
        }

        std::string new_path;
        new_path.reserve(path.size() + 32);

        for (const auto& key : matches) {

            new_path.clear();
            if (!path.empty()) {
                new_path = path;
                new_path.push_back('.');
            }
            new_path += key;

            if (!validate_node(
                    node.at(key),
                    child,
                    new_path,
                    depth + 1,
                    error_path))
            {
                return false;
            }
        }
    }

    return true;
}


bool JsonValidator::validate(const json& j) {
    std::vector<std::string> error_path;

    if (!schema.name.empty()) {

        const auto root_matches = find_patterns_in_json(schema.name, j);

        if (root_matches.empty()) {
            utils::error("Missing root key: " + schema.name, cxt, "", false, false);
            return false;
        }

        for (const auto& key : root_matches) {
            validate_node(j.at(key), schema, key, 0, error_path);
        }
    }
    else {
        validate_node(j, schema, "", 0, error_path);
    }

    if (!error_path.empty()) {
        auto parts = split_path(error_path.front());
        auto subtree = keep_only_path(j, parts);

        std::cout << "\n--- FAILED SUBTREE ---\n";
        std::cout << subtree.dump(4) << '\n';
        return false;
    }

    return true;
}


bool JsonValidator::validate_node(
    const json& node,
    const JsonValidator::Schema& schema,
    const std::string& path,
    int depth,
    std::vector<std::string>& error_path,
    bool report_error
) {
    switch (schema.type) {

        case Type::String:
            if (!node.is_string()) goto type_error;
            return true;

        case Type::Int:
            if (!node.is_number_integer()) goto type_error;
            return true;

        case Type::Bool:
            if (!node.is_boolean()) goto type_error;
            return true;

        case Type::Array: {
            if (!node.is_array()) goto type_error;

            const size_t n = node.size();

            if (schema.is_tuple) {
                if (n != schema.fields.size()) {
                    goto type_error;
                }

                for (size_t i = 0; i < n; i++) {
                    if (!validate_node(
                            node[i],
                            schema.fields[i],
                            path + "[" + std::to_string(i) + "]",
                            depth + 1,
                            error_path))
                        return false;
                }

                return true;
            }

            for (size_t i = 0; i < n; i++) {

                bool matched = false;

                for (const auto& field : schema.fields) {
                    std::vector<std::string> tmp;

                    if (validate_node(
                            node[i],
                            field,
                            path + "[" + std::to_string(i) + "]",
                            depth + 1,
                            tmp, 
                            false
                        ))
                    {
                        matched = true;
                        break;
                    }
                }

                if (!matched) {
                    if (error_path.empty())
                        error_path.emplace_back(format_json_path(path) + "[" + std::to_string(i) + "]");

                    if (report_error) utils::error(
                        "Array element does not match schema: " +
                        format_json_path(path) + "[" + std::to_string(i) + "]",
                        cxt,
                        "",
                        false,
                        false
                    );

                    return false;
                }
            }

            return true;
        }

        case Type::Object:
            if (!node.is_object()) goto type_error;
            return validate_children(node, schema, path, depth, error_path, report_error);
    }

    return true;

type_error:
    if (error_path.empty())
        error_path.emplace_back(path);

    if (report_error) utils::error(
        std::string("Type mismatch, expected a ") +
        schema.type_name() +
        " got a " +
        node.type_name() +
        " at: " +
        format_json_path(path),
        cxt,
        "",
        false,
        false
    );

    return false;
}


json load_and_validate_json(
    CompilerCxt& cxt,
    const std::string& filename,
    JsonValidator& validator
) {
    auto file_path = utils::get_file_path(filename, cxt);

    auto old = cxt.current_file;
    cxt.current_file = file_path;

    json data = json::parse(utils::read_file(file_path, cxt));

    if (!validator.validate(data))
        std::exit(1);

    cxt.current_file = old;

    return data;
}