
# Pratt Expression Parser

The expression parser uses the Pratt parsing technique (Top-Down Operator Precedence).

Operator definitions are loaded from the `expr definition` section of `parse_config.json`, allowing operators and their precedence to be configured without hardcoding every operator into the parser.

Implementation:

- `src/pratt_parser.cpp`
- `includes/pratt_parser.hpp`

## Configuration format

Each expression token entry must contain a `types` array.

Example:

```json
{
  "expr definition": {
    "identifier": {
      "types": ["value"]
    },
    "int_literal": {
      "types": ["value"]
    },
    "+": {
      "types": ["prefix", "infix"],
      "lbp": 50,
      "rbp": 51,
      "prefix bp": 100
    },
    "*": {
      "types": ["infix"],
      "lbp": 60,
      "rbp": 61
    },
    "(": {
      "types": ["opening wrapper"]
    },
    ")": {
      "types": ["closing wrapper"]
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

Every entry must refer to an existing lexer rule.

If a token cannot be found, or its entry is missing `types`, configuration loading reports an error.

## Supported token roles

The parser defines the following token roles:

| Role | Purpose |
|---|---|
| `value` | Literals and identifiers |
| `prefix` | Unary prefix operators |
| `infix` | Binary operators |
| `postfix` | Postfix operators |
| `ternary` | Ternary operators |
| `opening wrapper` | Opening grouping or call delimiter |
| `closing wrapper` | Closing delimiter |
| `argument separator` | Separates function arguments |
| `ternary separator` | Separates ternary branches |
| `expr terminator` | Ends the current expression |

A token can have multiple roles.

For example, `+` can be both a prefix and infix operator.

The current C configuration also uses `++` and `--` as prefix and postfix operators.

**Note:** Having a role defined in the enum does not guarantee that it has complete generic parsing behavior. Some operations currently have specialized implementations.

## Binding power

Binding power determines operator precedence.

The parser supports two configuration formats.

### Explicit binding powers

```json
"+": {
  "types": ["infix"],
  "lbp": 50,
  "rbp": 51
}
```

- `lbp`: Left binding power.
- `rbp`: Right binding power.

### Precedence and associativity

Alternatively:

```json
"^": {
  "types": ["infix"],
  "precedence": 70,
  "associativity": "right"
}
```

The parser converts precedence and associativity into binding powers.

The conversion is:

| Associativity | lbp | rbp |
|---|---|---|
| Left | precedence | precedence + 1 |
| Right | precedence + 1 | precedence |

Higher binding powers generally indicate tighter binding.

### Prefix operators

Prefix operators can specify:

```json
"-": {
  "types": ["prefix"],
  "prefix bp": 100
}
```

This value determines the binding power used when parsing the operand following the prefix operator.

### Postfix operators

Postfix operators can specify:

```json
"++": {
  "types": ["postfix"],
  "postfix bp": 100
}
```

The value is stored as the operator's left binding power.

## Expression parsing

The parser follows the general Pratt parsing structure:

1. Parse an initial atom.
2. Inspect the next token.
3. Determine whether it can extend the current expression.
4. Compare its binding power with the current right binding power.
5. Parse the operator and its operands.
6. Continue until an expression terminator or lower-precedence boundary is reached.

This allows expressions such as:

```text
a + b * c
```

to produce a tree equivalent to:

```text
    +
   / \
  a   *
     / \
    b   c
```

Multiplication binds more tightly than addition.

## Wrappers and function calls

The parser recognizes configured opening and closing wrappers.

Parentheses can group expressions:

```text
(a + b)
```

An opening wrapper following an existing expression is also used for function-call-like syntax:

```text
foo(a, b, c)
```

The implementation constructs a wrapper node containing the original expression and its arguments.

When an argument separator is encountered, the resulting node is labeled:

```text
func_call
```

Otherwise, the node is labeled:

```text
wrapper_contents
```

This behavior is currently specialized rather than being a completely generic postfix-wrapper framework.

## Ternary expressions

The parser has special handling for ternary expressions.

The configuration uses:

- `ternary` for the conditional operator.
- `ternary separator` for the separator between branches.

For example:

```text
condition ? true_value : false_value
```

The ternary parser builds an AST node containing the condition and both branches.

The ternary operator requires binding-power configuration.

## Expression terminators

An expression terminator tells the Pratt parser to return the expression parsed so far without consuming the terminator.

For example:

```json
";": {
  "types": ["expr terminator"]
}
```

Given:

```text
result = a + b;
```

The Pratt parser returns the expression before the semicolon.

The enclosing grammar rule is responsible for consuming the semicolon.

## Adding an operator

To add an operator:

1. Add its token to the lexer configuration.
2. Add its label to `expr definition`.
3. Specify its parser role.
4. Configure its binding power or precedence and associativity where required.
5. Add test expressions covering precedence and associativity.
6. Verify its parsing behavior in `src/pratt_parser.cpp`.

For example, adding an infix operator:

```json
"**": {
  "types": ["infix"],
  "precedence": 80,
  "associativity": "right"
}
```

Configuration alone does not guarantee that every operator role is implemented end-to-end.

## Current limitations

- Some operator roles have specialized parsing behavior.
- Wrapper handling is not fully generic.
- Function calls use special handling inside the expression parser.
- Postfix and prefix behavior should be checked against the current implementation before extending the operator system.
- The parser does not perform type checking or semantic analysis.

The implementation in `src/pratt_parser.cpp` is the definitive reference for actual parsing behavior.
