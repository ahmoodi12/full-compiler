
# Grammar Parser

The grammar parser matches a token stream against rules loaded from `parse_config.json`.

It uses the lexer to resolve token labels, delegates expression parsing to the Pratt parser, and converts successful statement matches into AST nodes.

Implementation:

- `src/parser.cpp`
- `includes/parser.hpp`
- `includes/ast.hpp`

## Configuration layout

The parser configuration contains two major sections:

- `expr definition`: Defines expression token roles and binding powers.
- `grammar`: Contains reusable variables and statement rules.

The included C-like language configuration is located at:

```text
config/C/parse_config.json
```

A simplified example:

```json
{
  "grammar": {
    "variables": {
      "#block": {
        "pattern": [
          ["{"],
          {"repeat": "__stmt__"},
          ["}"]
        ]
      }
    },
    "statement rules": {
      "return_stmt": {
        "pattern": [
          ["return"],
          {"optional": ["__expr__"]},
          [";"]
        ]
      }
    }
  }
}
```

Each grammar rule contains a `pattern`, which is an ordered list of pattern elements.

## Pattern elements

### Token sequences

An array represents a sequence of elements that must match in order.

```json
["if", "(", {"condition": "__expr__"}, ")"]
```

Strings are resolved against lexer rules.

### Captures

An object maps a capture name to the token label or parser construct being matched.

```json
{"condition": "__expr__"}
```

Captured values are stored in the resulting AST node's `captures` map.

Captures can contain:

- Lexer tokens.
- Expressions.
- Nested statements.
- Grammar variables.

### Optional paths

An object containing `optional` creates an alternate grammar path.

```json
{
  "optional": ["else", "#block"]
}
```

The parser expands this into another internal rule based on the preceding pattern.

This allows optional constructs such as:

- Optional initializers.
- Optional return expressions.
- Optional else blocks.

The current implementation expects an object containing the `optional` property at that position in the pattern.

### Repetition

A repetition object uses the following format:

```json
{
  "repeat": "#func_arg",
  "separator": ","
}
```

Fields:

- `repeat`: The token, variable, or parser construct to match repeatedly.
- `separator`: Optional lexer token consumed between repeated items.

The correct spelling is `separator`, not `seperator`.

**Important implementation limitation:** The current `repeat()` implementation consumes a configured separator after each successful item, including the final item before a following stop token. Consequently, separator-based repetition currently behaves as though the final item also requires a separator.

Do not assume conventional comma-separated list behavior until this implementation is corrected and covered by tests.

Repetition without separators also has edge cases involving failed matches and AST accumulation.

### Reusable variables

Rules under `grammar.variables` can be referenced by their names.

Examples:

```text
#block
#func_arg
```

Variable rules are matched separately from top-level statement rules.

### Special parser constructs

The grammar parser recognizes two special labels.

#### `__expr__`

Invokes the Pratt expression parser.

Its resulting AST is stored as a capture when a capture name is provided. Otherwise, it is appended to the expression list of the current statement match.

Example:

```json
{"value": "__expr__"}
```

#### `__stmt__`

Attempts to match a top-level statement rule.

Its resulting AST is stored as a capture or appended to the current match's child statements.

Example:

```json
{
  "repeat": "__stmt__"
}
```

Other labels must resolve to lexer tokens or declared grammar variables.

Unknown labels cause a matching failure.

## Matching and rule selection

The parser uses speculative matching.

For each input position, `try_all_statements()` attempts every rule in the selected rule collection.

Each attempt:

1. Records the starting token position.
2. Attempts to match the rule.
3. Records how many tokens were consumed.
4. Restores the original token position.

The parser then selects the valid match consuming the most tokens.

If no rule succeeds, the parser reports the error associated with the longest attempted match.

When two valid matches consume the same number of tokens, the later match replaces the earlier candidate.

## AST construction

Each successful grammar match becomes an `ASTNode`.

The resulting node contains:

- The grammar rule name as its token label.
- Named captures in `ASTNode::captures`.
- Expression nodes in its children.
- Nested statement nodes in its children.

For example:

```text
return_stmt
└── expression
    └── addition
        ├── identifier
        └── integer_literal
```

Captured lexer tokens retain their token IDs, labels, source text, and source locations.

## Current limitations

- The parser is still a prototype.
- Not every grammar shape or repetition case is fully reliable.
- Separator-based repetition has implementation edge cases.
- The grammar JSON schema validates structure and types, but token and variable references also require runtime lookup.

When modifying grammar behavior, use `src/parser.cpp` and `config/C/parse_config.json` as the primary references.
