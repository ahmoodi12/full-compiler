# JsonValidator

A lightweight recursive JSON schema validator built on top of  
[`nlohmann::json`](https://github.com/nlohmann/json).

It is designed for **simple schema definitions**, **fast validation**, and **clear error reporting**.

---

## What it does

JsonValidator helps you:
- Check JSON types (`string`, `int`, `bool`, `array`, `object`)
- Validate nested structures
- Validate arrays element-by-element
- Match keys using regex (very useful for dynamic JSON)
- Allow optional fields
- Show exactly where validation fails (including a trimmed JSON subtree)

---

## Features

### ✔ Type Validation

Supports strict type checking:

- `String`
- `Int`
- `Bool`
- `Array`
- `Object`

If a value does not match the expected type, validation fails immediately for that path.

---

## Schema-Based Validation

Schemas are defined using a simple recursive structure:

```cpp
CompilerCxt cxt;

cxt.current_file = utils::get_file_path(
    "C:/projects/full compiler/data/c frontend data/lex_data.json"
);

JsonValidator validator(cxt, {
    "",
    JsonValidator::Type::Object,
    {
        {
            "regexes",
            JsonValidator::Type::Object,
            {
                {
                    "\\d+",
                    JsonValidator::Type::Int
                }
            }
        }
    }
});

```

## Example JSON
```cpp
{
    "regexes": {
        "10": "for",
        "hello": "293"
    }
}
```


### What happens during validation

The schema above says:

regexes must be an object
inside regexes, all values of keys that match "\d+" must be Int.

So:

"10" matches \\d+
but its value is "for" → ❌ invalid (not an int)

---

## Error Output

When validation fails, you get a clear error message:

```powershell
Error in file 'C:/projects/full compiler/data/c frontend data/lex_data.json':
>> Type mismatch (expected int): regexes.10 <<

--- FAILED SUBTREE ---
{
    "regexes": {
        "10": "for"
    }
}

```

- where: `"10": "for"`
- why `Type mismatch (expected int)`