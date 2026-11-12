
# JSON Validator

The project includes a recursive JSON structure validator built on top of `nlohmann/json`.

It is used to validate lexer and parser configuration files before those files are loaded into their respective components.

Implementation:

- `includes/json_validator.hpp`
- `src/json_validator.cpp`

## Supported types

The validator currently supports five schema types.

| C++ enum | Accepted JSON value |
|---|---|
| `JsonType::String` | String |
| `JsonType::Int` | Integer number |
| `JsonType::Bool` | Boolean |
| `JsonType::Array` | Array |
| `JsonType::Object` | Object |

Floating-point numbers and JSON `null` do not have dedicated schema types.

Integer validation uses `is_number_integer()`, so decimal numbers are not accepted as integers.

## Schema structure

A schema node contains:

- `name`: Property name or regular expression used to select object keys.
- `type`: Expected `JsonType`.
- `fields`: Child schemas.
- `optional`: Whether the matching property can be absent.
- `is_tuple`: Whether an array must have a fixed structure.

A schema can be constructed directly in C++.

```cpp
using Schema = JsonValidator<nlohmann::json>::Schema;
using Type = JsonType;

Schema schema{
    "",
    Type::Object,
    {
        Schema{
            "numbers",
            Type::Object,
            {
                Schema{"[a-z]+", Type::Int}
            }
        }
    }
};

JsonValidator<nlohmann::json> validator(cxt, schema);

bool valid = validator.validate(data);
```

An empty root name means the supplied JSON value itself is validated.

Object property names are matched using `std::regex_match`, meaning the entire key must match the regular expression.

In this example:

- `numbers` must be an object.
- Every matching lowercase alphabetic key must contain an integer.
- Unknown object keys are rejected.

## Object validation

For each child schema, the validator:

1. Finds all object keys matching the child's name pattern.
2. Fails if no matching keys exist and the field is required.
3. Validates every matching property's value.
4. Rejects object keys that were not matched by any child schema.

This makes regex-based schemas useful for validating maps with constrained key formats.

For example:

```cpp
Schema{"\\d+", Type::String}
```

matches keys consisting entirely of digits.

### Optional properties

A property can be made optional by setting the fourth constructor argument to `true`.

```cpp
Schema{"description", Type::String, {}, true}
```

Optional means the property can be absent.

If the property exists, its value must still match the specified schema.

## Array validation

The validator supports two array modes.

### Tuple mode

When `is_tuple` is true:

- The array length must equal `fields.size()`.
- Every element is validated against the corresponding schema.
- Element order matters.

For example:

```json
["identifier", "[a-z]+", false]
```

This is how the lexer configuration validates its three-element token definitions.

### Alternative-element mode

When `is_tuple` is false:

- Every array element must match at least one schema in `fields`.
- The array can contain different element types if those types are described by the supplied schemas.

## Error reporting

Validation returns `false` when a schema check fails.

Diagnostics are reported through `utils::error()`.

Possible errors include:

- Missing required property.
- Unknown object property.
- Type mismatch.
- Incorrect tuple length.
- Array element matching none of the allowed schemas.

The validator tracks the failing JSON path.

Paths are displayed using:

```text
"grammar" -> "statement rules" -> "return"
```

Array indices are represented using:

```text
"tokens" -> "32" -> [1]
```

When validation fails, the validator also prints a `FAILED SUBTREE` JSON excerpt containing the relevant portion of the input.

## Loading validated JSON

The project provides `load_and_validate_json<Json>()` for reading, parsing, and validating configuration files.

Example:

```cpp
JsonValidator<nlohmann::json> validator(cxt, schema);

auto config = load_and_validate_json<nlohmann::json>(
    cxt,
    "config/C/lex_config.json",
    validator
);
```

The helper:

1. Reads the configuration file.
2. Parses it using nlohmann/json.
3. Validates it against the supplied schema.
4. Terminates with a non-zero exit status if validation fails.

It also temporarily updates `CompilerCxt::current_file` so diagnostics can identify the configuration file.

## Limitations

- No dedicated float, null, enum, or numeric-range schema types.
- Regex patterns match complete object keys.
- Unknown object keys are rejected.
- The validator checks JSON structure and types, not every application-specific relationship between fields.
- Additional semantic checks may be performed by the lexer or parser after validation.
 