
# Lexer

The lexer converts source text into a sequence of `Token` objects.

Its rules are loaded from JSON, allowing token IDs, labels, regular expressions, and skip behavior to be configured without modifying the lexer implementation.

Implementation:

- `src/lexer.cpp`
- `includes/lexer.hpp`

Configuration:

- `config/C/lex_config.json`

## Configuration format

The lexer configuration contains a top-level `tokens` object.

Each property name represents a numeric token ID. Its value is a three-element array.

```json
{
  "tokens": {
    "32": ["identifier", "[a-zA-Z_][a-zA-Z0-9_]*", false],
    "33": ["int_literal", "[0-9]+", false],
    "17": ["whitespace", "[ \\t\\r]+", true]
  }
}
```

The three array elements are:

| Position | Meaning |
|---|---|
| 1 | Token label |
| 2 | Regular-expression pattern |
| 3 | Skip flag |

The skip flag determines whether the token is included in the resulting token vector.

- `false`: Emit the token.
- `true`: Consume the input but discard the token.

Token IDs are parsed as integers.

Labels should be unique because parser configuration refers to lexer rules using their labels.

## Regular expressions

Patterns use the C++ standard library's `std::regex` implementation with ECMAScript syntax.

The lexer automatically prefixes each pattern with `^`.

This means every match must begin at the current input position.

For example:

```json
"identifier": [
  "identifier",
  "[a-zA-Z_][a-zA-Z0-9_]*",
  false
]
```

Matches identifiers such as:

```text
hello
variable_name
_variable
value123
```

## Matching algorithm

At each input position, the lexer:

1. Tries every configured rule against the remaining input.
2. Selects the rule matching the most characters.
3. If multiple rules match the same number of characters, selects the rule with the lowest numeric token ID.
4. Emits the token unless its skip flag is enabled.
5. Advances by the matched length and repeats.

This is a longest-match strategy.

For example, if both `=` and `==` are configured, the lexer selects `==` when the input begins with `==`, because it matches more characters.

Token ID is used as a tie-breaker when match lengths are equal.

## Token structure

A token contains:

- `id`: Numeric token ID.
- `label`: Configured token label.
- `data`: Exact matched source text.
- `loc`: Source file, line, and column information.
- `skip`: Whether the token is discarded.

Skipped tokens still consume their matching input.

Common examples include:

- Whitespace
- Newlines
- Comments

## Adding a token

To add a token:

1. Choose an unused numeric ID.
2. Add an entry to `config/C/lex_config.json`.
3. Give it a descriptive label.
4. Add a regular expression.
5. Decide whether the token should be skipped.
6. If the parser needs the token, reference its label in `parse_config.json`.

For example, a semicolon token can be configured and then used as an expression terminator and in statement grammar patterns.

## Running the lexer

The current executable runs both the lexer and parser.

Build:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Run:

```sh
./build/compiler_main path/to/program.c config/C
```

The executable prints the token stream before printing the AST.

The lexer also exposes `print_output()` for displaying token IDs, labels, locations, and token contents.

## Current limitations

- The lexer uses `std::regex`; it is not a stateful scanner with custom lexical modes.
- There is no separate token-priority field.
- Longest match wins, followed by the lowest numeric token ID.
- The lexer does not synthesize an EOF token when the input ends.
- The configured `EOF` entry in the sample configuration is not a general EOF mechanism.
