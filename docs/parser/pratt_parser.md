# PrattParser

A lightweight **Pratt parser implementation** for building Abstract Syntax Trees (ASTs) from tokens using configurable operator precedence rules loaded from JSON.

It supports:
- Prefix / infix / postfix operators
- Function calls (`f(x, y)`)
- Parentheses / wrappers (`(...)`)
- Dynamic grammar defined via JSON
- Lexer-rule integration

---

## Overview

This parser is based on the **Pratt parsing technique**, where each token type defines its own parsing behavior through:
- **Null denotation (nud)** → handled in `parse_atom()`
- **Left denotation (led)** → handled in `parse_expr()`

Instead of hardcoding grammar rules, this implementation loads them from JSON, making the parser extensible without recompiling.

---

## Features

### Expression Support
- Infix operators with precedence
- Prefix operators (e.g. `-a`, `!a`)
- Function calls: `foo(a, b, c)`
- Parentheses grouping: `(expr)`
- Expression terminators (e.g. `,` or `;` depending on grammar)

### Rule System
Each token is mapped to a rule:
- `Value` → literal values (numbers, identifiers)
- `Prefix` → unary prefix operators
- `Infix` → binary operators
- `Postfix` → trailing operators
- `Ternary` → conditional operators
- `OpeningWrapper` / `ClosingWrapper` → grouping / calls
- `ExprEnd` → argument separators

---

## Architecture

### Core Components

- **Token stream**
  - Managed via `tokens`, `pos`, `peek()`, `consume()`

- **Rule system**
  - `Rule` defines how each token behaves
  - Stored in:
    - `by_id`
    - `by_label`

- **AST output**
  - Built using `ASTNode`
  - Uses `std::unique_ptr` for children

---

## JSON Grammar Format

Example:

```json
{
  "prefix binding power": 100,
  "expr data": {
    "+": {
      "types": ["infix"],
      "precedence": 10,
      "associativity": "left"
    },
    "-": {
      "types": ["prefix", "infix"],
      "precedence": 10,
      "associativity": "left"
    },
    "(": {
      "types": ["opening wrapper"]
    },
    ")": {
      "types": ["closing wrapper"]
    },
    ",": {
      "types": ["expr terminator"]
    }
  }
}