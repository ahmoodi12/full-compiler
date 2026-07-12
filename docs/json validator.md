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

JsonValidator validator(cxt, {
    "",
    JsonValidator::Type::Object,
    {
        {
            "numbers",
            JsonValidator::Type::Object,
            {
                {
                    "[a-z]+",
                    JsonValidator::Type::Int
                }
            }
        }
    }
});

```

## Equivalent JSON
```cpp
{
    "numbers": {
        "hello": 90,
    }
}
```


### What happens during validation

The schema above says:

`numbers` must be an object. inside it, all values that match `"[a-z]+"` must be a int.

So:

`"hello"` matches `[a-z]+`

and its value `90` is a int therefore also valid ✅

---

## Error Output

When validation fails, you get a clear error message:

> <span style="color:orange">Error</span> in file '<span style="color:cyan">C:/projects/full compiler/data/c frontend data/lex_data.json</span>':
>
> <span style="color:blue">Type mismatch (expected int): "regexes" -> "10" <<</span>
> ```
> --- FAILED SUBTREE ---
> {
>     "regexes": {
>         "10": "for"
>     }
> }
> ```
(it looks better in a terminal in my opinion)

- where: `"10": "for"`
- why `Type mismatch (expected int)`