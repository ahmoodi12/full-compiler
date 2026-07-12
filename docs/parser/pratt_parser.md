# Pratt Parser

This parser uses **Pratt parsing (Top-Down Operator Precedence)** to parse expressions into an AST. Operator behavior is driven by a configuration file instead of being hardcoded, making it easy to add or modify operators.

## Configuration

The parser reads operator information from a JSON configuration.

Example:

```json
{
  "prefix binding power": 100,

  "expr definition": {
    "(": {
      "types": ["opening wrapper"]
    },

    ")": {
      "types": ["closing wrapper"]
    },

    "identifier": {
      "types": ["value"]
    },

    "int_literal": {
      "types": ["value"]
    },

    "+": {
      "lbp": 50,
      "rbp": 51,
      "types": ["prefix", "infix"]
    },

    "*": {
      "lbp": 60,
      "rbp": 61,
      "types": ["infix"]
    },

    "<": {
      "lbp": 40,
      "rbp": 41,
      "types": ["infix"]
    },

    "==": {
      "lbp": 30,
      "rbp": 31,
      "types": ["infix"]
    },

    "?": {
      "lbp": 20,
      "rbp": 19,
      "types": ["ternary"]
    },

    ":": {
      "types": ["ternary separator"]
    },

    ",": {
      "types": ["argument separator"]
    },

    ";": {
      "types": ["expr terminator"]
    }
  }
}
```

## Binding Power

Operators can be described using **left/right binding power**:

```json
"+": {
  "lbp": 50,
  "rbp": 51,
  "types": ["infix"]
}
```

or equivalently using **precedence** and **associativity**:

```json
"?": {
  "precedence": 20,
  "associativity": "left",
  "types": ["ternary"]
}
```

Internally these represent the same information. If `precedence` and `associativity` are provided, they are converted into `lbp` and `rbp` before parsing.

## Token Types

The parser recognizes several token roles:

* `value` – literals and identifiers
* `prefix` – unary operators
* `infix` – binary operators
* `ternary` – ternary operator (`?`)
* `opening wrapper` / `closing wrapper` – grouping tokens like `(` and `)`
* `ternary separator` – `:`
* `argument seperator` – `,`
* `expr terminator` – `;`

A token may have multiple roles (for example, `+` is both a prefix and infix operator).

## Extending

To add a new operator, simply add an entry to the configuration with its type and binding power (or precedence/associativity). No parser logic needs to be changed unless the operator requires special parsing behavior.
