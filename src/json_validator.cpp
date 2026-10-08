#include "json_validator.hpp"
#include "compiler_cxt.hpp"
#include "utils.hpp"

#include <filesystem>
#include <regex>

template <typename Json>
Json keep_only_path(
    const Json& node,
    const std::vector<JsonPathPart>& path,
    size_t idx = 0
) {
    if (idx >= path.size())
        return node;

    const auto& part = path[idx];

    if (part.type == JsonPathPart::Type::Index) {

        Json arr = Json::array();

        if (node.is_array() && part.index < node.size()) {
            arr.push_back(
                keep_only_path(node[part.index], path, idx + 1)
            );
        }

        return arr;
    }

    if (node.is_object()) {

        Json obj;

        auto it = node.find(part.key);

        if (it != node.end()) {
            obj[part.key] =
                keep_only_path(*it, path, idx + 1);
        }

        return obj;
    }

    return node;
}

std::string format_json_path(
    const std::vector<JsonPathPart>& path
) {
    std::string out;

    for (size_t i = 0; i < path.size(); i++) {
        if (i != 0)
            out += " -> ";

        const auto& part = path[i];

        if (part.type == JsonPathPart::Type::Key) {
            out += "\"";
            out += part.key;
            out += "\"";
        }
        else {
            out += "[";
            out += std::to_string(part.index);
            out += "]";
        }
    }

    return out;
}

template <typename Json>
const char* JsonValidator<Json>::Schema::type_name() const {
    switch (type)
    {
    case JsonType::String:  return "string";
    case JsonType::Int:     return "int";
    case JsonType::Bool:    return "bool";
    case JsonType::Array:   return "array";
    case JsonType::Object:  return "object";
    }

    return "unknown";
}

template <typename Json>
std::vector<std::string> JsonValidator<Json>::find_patterns_in_json(
    const std::string& pattern,
    const Json& j
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


template <typename Json>
bool JsonValidator<Json>::validate_children(
    const Json& node,
    const Schema& schema,
    const std::vector<JsonPathPart>& path,
    std::vector<JsonPathPart>& error_path,
    bool report_error
    ) {
    std::unordered_set<std::string> matched_keys;

    for (const auto& child : schema.fields) {
        const auto matches = find_patterns_in_json(child.name, node);

        if (matches.empty()) {
            if (child.optional) continue;

            if (error_path.empty()) {
                error_path = path;
            }

            if (report_error) {
                utils::error("Missing field (pattern): " + child.name, cxt, "", false, false);
            }

            return false;
        }

        for (const auto& key : matches) {
            matched_keys.insert(key);

            auto new_path = path;
            new_path.push_back(JsonPathPart::key_part(key));

            if (!validate_node(node.at(key), child, new_path, error_path)) {
                return false;
            }
        }
    }

    for (const auto& [key, value] : node.items()) {
        if (matched_keys.find(key) != matched_keys.end()) continue;

        auto new_path = path;
        new_path.push_back(JsonPathPart::key_part(key));

        if (error_path.empty()) {
            error_path = new_path;
        }

        if (report_error) {
            utils::error("Unknown field: " + format_json_path(new_path), cxt, "", false, false);
        }

        return false;
    }

    return true;
}

template <typename Json>
bool JsonValidator<Json>::validate(const Json& j) {
    std::vector<JsonPathPart> error_path;

    if (!schema.name.empty()) {

        const auto root_matches = find_patterns_in_json(schema.name, j);

        if (root_matches.empty()) {
            utils::error("Missing root key: " + schema.name, cxt, "", false, false);
            return false;
        }

        for (const auto& key : root_matches) {
            std::vector<JsonPathPart> path;
            path.push_back(JsonPathPart::key_part(key));

            validate_node(
                j.at(key),
                schema,
                path,
                error_path
            );
        }
    }
    else {
        std::vector<JsonPathPart> path;

        validate_node(
            j,
            schema,
            path,
            error_path
        );    
    }

    if (!error_path.empty()) {
        auto subtree = keep_only_path(j, error_path);

        std::cout << "\n--- FAILED SUBTREE ---\n";
        std::cout << subtree.dump(4) << '\n';
        return false;
    }

    return true;
}

template <typename Json>
bool JsonValidator<Json>::validate_node(
    const Json& node,
    const Schema& schema,
    const std::vector<JsonPathPart>& path,
    std::vector<JsonPathPart>& error_path,
    bool report_error
    ) {
    switch (schema.type) {

        case JsonType::String:
            if (!node.is_string()) goto type_error;
            return true;

        case JsonType::Int:
            if (!node.is_number_integer()) goto type_error;
            return true;

        case JsonType::Bool:
            if (!node.is_boolean()) goto type_error;
            return true;

        case JsonType::Array: { // the brackets are so that the compiler knows that const size_t n = node.size(); is local to array
            if (!node.is_array()) goto type_error;

            const size_t n = node.size();
            if (schema.is_tuple) {
                if (n != schema.fields.size()) {
                    goto type_error;
                }

                for (size_t i = 0; i < n; i++) {
                    auto new_path = path;
                    new_path.push_back(
                        JsonPathPart::index_part(i)
                    );

                    if (!validate_node(
                            node[i],
                            schema.fields[i],
                            new_path,
                            error_path))
                        return false;
                }
                return true;
            } 

            for (size_t i = 0; i < n; i++) {

                bool matched = false;

                for (const auto& field : schema.fields) {
                    std::vector<JsonPathPart> tmp;

                    auto new_path = path;
                    new_path.push_back(
                        JsonPathPart::index_part(i)
                    );

                    if (validate_node(
                            node[i],
                            field,
                            new_path,
                            tmp, 
                            false
                        ))
                    {
                        matched = true;
                        break;
                    }
                }

                if (!matched) {
                    if (error_path.empty()) {
                        error_path = path;
                        error_path.push_back(
                            JsonPathPart::index_part(i)
                        );
                    }

                    if (report_error) {
                        utils::error(
                        "Array element does not match schema: " +
                        format_json_path(path) + " -> [" + std::to_string(i) + "]",
                        cxt,
                        "",
                        false,
                        false
                        );
                    }
                    return false;
                }
            }
            return true;
        }
        
        case JsonType::Object:
            if (!node.is_object()) goto type_error;
            return validate_children(node, schema, path, error_path, report_error);
    
        default:
            error_path = path;
            if (report_error) {
                utils::error("got an unknown type '" + std::string(schema.type_name()) + "'.", cxt, "", false, false);
            }
            return false;
    }

    return true;

type_error:
    if (error_path.empty()){
        error_path = path;
    }

    if (report_error) {
        utils::error(
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
    }

    return false;
}

template class JsonValidator<nlohmann::json>;
template class JsonValidator<nlohmann::ordered_json>;