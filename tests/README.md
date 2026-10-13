# Front-end test corpus

## Directories

- `lexer/`: valid snippets intended to exercise token recognition and maximal-munch behavior.
- `parser/`: expression-focused inputs for the Pratt parser.
- `grammar/`: C-like statement/grammar examples.
- `invalid/`: malformed snippets that should be rejected or report a lexical/parser diagnostic.

## Important integration notes

1. The lexer config uses the schema implemented in `includes/lexer.hpp`:
   `{"rules": {"id": ["LABEL", "regex", skip_boolean]}}`.
2. `Lexer::run()` selects the longest match; equal-length ties prefer the smaller numeric rule ID.
3. The sample lexer emits identifiers rather than keyword-specific tokens. If your grammar expects `INT`, `IF`, `RETURN`, etc. as token labels, add dedicated keyword rules before `IDENTIFIER` and ensure keyword rules win equal-length ties by using lower IDs.
4. The parser's expression config uses token labels resolved by `Lexer::find_lex_rule()`.
5. The grammar parser is documented as in progress. The grammar config is a starter template and should be validated against the current implementation before treating it as production-ready.
6. Invalid lexer input currently appears to report an error and stop lexing; tests should verify diagnostics and behavior rather than assume exceptions.

## Suggested test protocol

For each lexer fixture:
- Run the lexer.
- Compare the ordered sequence of `(label, data)` tokens to a golden expectation.
- Confirm skipped whitespace/comments are absent.
- Confirm multi-character operators win over their prefixes.

For Pratt fixtures:
- Assert successful parse for valid expressions.
- Inspect AST shape to verify precedence and associativity.
- Assert clear failures for missing operands, wrappers, and ternary separators.

For grammar fixtures:
- Keep parser tests isolated until the generic grammar/repetition path is stable.
- Add golden AST tests for nested blocks, optional `else`, empty parameter lists, and comma-separated parameters.
