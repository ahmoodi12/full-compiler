# Parser Documentation

## Overview

The parser is a grammar-driven parser built on top of the lexer. Grammar rules are described using JSON and compiled into parser rules at startup.

The parser has three main responsibilities:

1. Match grammar rules against the token stream.
2. Parse expressions through the Pratt parser.
3. Convert successful matches into AST nodes.

The parser is intentionally generic: language-specific constructs should primarily be expressed in the grammar rather than implemented as special cases in C++.

## Grammar Structure

Grammar definitions are divided into:

* `statement rules` — rules that can form statements.
* `variables` — reusable grammar rules that can be referenced by other rules.

For example:

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
    }
  }
}
```

A grammar rule consists of a `pattern`, which is a sequence of pattern elements.

## Pattern Elements

### Token sequences

An array represents a sequence of tokens:

```json
["if", "(", "__expr__", ")"]
```

Each element is matched in order.

### Captures

An object represents a captured grammar element:

```json
{"cond": "__expr__"}
```

The key is the capture name and the value is the token or grammar construct being matched.

Captured lexer tokens are stored in the resulting `StmtMatch`.

### Optional paths

An object containing `optional` creates an alternate path through the grammar:

```json
{
  "optional": ["else", "#block"]
}
```

The optional path is represented internally as another parser rule whose `parent_i` points to the original rule.

Optional paths are currently restricted according to the `allow_optionals` parameter. Variables do not permit optional paths.

### Repetition

Repetition is the generic mechanism for matching a grammar construct zero or more times:

```json
{
  "repeat": "declaration"
}
```

A separator can optionally be specified:

```json
{
  "repeat": "declaration",
  "seperator": ","
}
```

This allows constructs such as:

```text
foo(int x, int y, int z)
```

without requiring a dedicated `__declarations__` parser construct.

The intention is for repetition to replace specialized constructs such as:

```text
__exprs__
__statements__
__declarations__
```

with one generic mechanism.

## Parser Meta-Constructs

Some labels have special meaning to the parser.

### `__expr__`

`__expr__` invokes the Pratt parser:

```json
{"cond": "__expr__"}
```

The resulting expression AST is stored in `StmtMatch::exprs`.

### `__stmt__`

`__stmt__` represents a generic statement and should invoke statement-rule matching.

This is particularly useful with repetition:

```json
{
  "repeat": "__stmt__"
}
```

For example, a block can therefore be represented as:

```json
{
  "pattern": [
    ["{"],
    {"repeat": "__stmt__"},
    ["}"]
  ]
}
```

### Deprecated/superseded constructs

Earlier versions used specialized constructs such as:

```text
__exprs__
__statements__
__declarations__
```

These do not scale well because every new repeated grammar concept requires another parser-specific construct.

They should be replaced by `repeat`.

## Matching

`match_stmt()` performs speculative matching.

It records the current token position, attempts to match a rule, and reports the number of tokens consumed:

```cpp
result.size = pos - start_pos;
```

The parser then restores the original position:

```cpp
pos = start_pos;
```

This allows callers to compare multiple possible rules before committing to one.

A successful caller commits a match with:

```cpp
pos += match.size;
```

## Longest Match

`parse_statement()` attempts every statement rule at the current token position.

The parser tracks:

* the longest attempted match;
* the longest valid match.

The longest valid rule wins.

This allows grammars with overlapping prefixes to be resolved without hardcoding individual statement types into the parser.

Conceptually:

```text
input
  │
  ├── rule A → valid, 3 tokens
  ├── rule B → valid, 7 tokens
  └── rule C → invalid, 9 tokens
             │
             ↓
       choose rule B
```

If no rule is valid, the parser can use the longest attempted match to provide a useful error.

## Repetition Semantics

`repeat` should follow this general model:

```text
check termination
      │
      ├── terminated → success
      │
      └── not terminated
              │
              ↓
        match repeated rule
              │
        ┌─────┴─────┐
        │           │
      fail        success
        │           │
      error      consume
                    │
                    ↓
             optional separator
                    │
                    ↓
                repeat
```

For a separator-based repetition:

```json
{
  "repeat": "declaration",
  "seperator": ","
}
```

the intended syntax is:

```text
declaration
, declaration
, declaration
```

rather than requiring a trailing separator.

Thus:

```text
foo(int x, int y)
```

is valid, while:

```text
foo(int x, int y,)
```

should not be accepted unless the grammar explicitly gains support for trailing separators.

## AST Construction

`parse_stmt()` converts a successful `StmtMatch` into an `ASTNode`.

The resulting node contains:

* the grammar rule's statement name;
* captured tokens;
* expression children;
* sub-statement children.

For example:

```text
assignment
├── lhs capture
├── expression
└── ...
```

Repeated statements should be accumulated into `sub_stmts` so that a block such as:

```text
{
    foo();
    bar();
}
```

produces multiple child AST nodes rather than discarding repeated matches.

## Current Design Goal

The parser should remain mostly grammar-agnostic.

Prefer:

```json
{
  "repeat": "some_rule",
  "seperator": ","
}
```

over adding a C++ parser special case such as:

```text
__some_rules__
```

The parser's built-in constructs should describe **parser behavior**, not individual language constructs.

The desired architecture is:

```text
JSON grammar
     │
     ↓
Parser rules
     │
     ↓
generic matching
     │
     ├── token
     ├── variable
     ├── expression
     ├── optional
     └── repeat
             │
             ↓
            AST
```

## Known Work Remaining

* Complete `parse_statement()`.
* Integrate `__stmt__` with generic statement matching.
* Finish `repeat()` accumulation and position handling.
* Ensure repeated matches become AST sub-statements.
* Define precise zero-or-more semantics.
* Decide how repetition determines its termination condition.
* Rename `seperator` to `separator` if the grammar format is not intended to preserve the existing spelling.
* Add parser tests for empty and non-empty repetitions.
* Add tests for separator-based repetition.
* Add tests for nested repetition and blocks.





# Parser Changelog

## Unreleased

### Changed

* Reworked the grammar design around a generic `repeat` construct.
* Moved away from specialized repeated grammar constructs such as `__exprs__`, `__statements__`, and `__declarations__`.
* Repetition is now intended to work with arbitrary grammar rules.
* Added optional separator support to repeated grammar rules.
* Added the foundation for generic statement repetition through `__stmt__`.
* Continued using speculative rule matching and longest-valid-match selection for statement parsing.

### Grammar

Repeated constructs can now be expressed as:

```json
{
  "repeat": "declaration",
  "seperator": ","
}
```

rather than introducing a dedicated parser construct for declaration lists.

Blocks can similarly use:

```json
{
  "repeat": "__stmt__"
}
```

### In Progress

* Finish integration of `repeat()` into `match_stmt()`.
* Finish generic `__stmt__` handling.
* Complete `parse_statement()`.
* Preserve repeated matches in the resulting AST.
* Establish final repetition termination and separator semantics.
* Add parser regression tests.

### Design Direction

The parser is being kept deliberately generic.

New grammar concepts should preferably be expressible through combinations of existing primitives rather than requiring new hardcoded `__something__` constructs in the C++ parser.
