
# Full Compiler

A small, experimental compiler frontend written in C++20. The current implementation focuses on a configurable lexer, JSON-driven grammar rules, Pratt expression parsing, and AST construction.

The project is a learning and experimentation codebase. It is **not yet a complete compiler**: semantic analysis, intermediate representation, optimization, code generation, and a virtual machine are not currently implemented.

## What is implemented

- **JSON configuration validation** for lexer and parser configuration files.
- **Configurable lexer** using regular expressions declared in JSON.
- **Grammar parser** whose statement and reusable-variable rules are declared in JSON.
- **Pratt expression parser** with configurable token roles and binding powers.
- **AST construction and printing** for parsed statements and expressions.

The executable currently prints the token stream and AST. Although the CLI accepts an output-file option, the current `main.cpp` does not yet emit compiled output.

## Repository layout

```text
.
├── config/
│   └── C/
│       ├── lex_config.json
│       └── parse_config.json
├── docs/
│   ├── json validator.md
│   ├── lexer.md
│   └── parser/
│       ├── parser.md
│       └── pratt_parser.md
├── includes/
├── src/
├── tests/
├── CMakeLists.txt
└── run_tests.ps1
```

## Build

Requirements:

- CMake 3.15 or newer
- A C++20 compiler
- A supported build tool, such as Ninja

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

This builds the `compiler_main` target.

**Testing note:** The checked-in CMake configuration currently searches for test sources under `test/*.cpp`, while the repository's test sources are under `tests/`. The PowerShell script also expects a pre-existing `cm.exe`. Test automation may therefore need maintenance.

## Run the frontend

Run from the repository root so relative configuration paths resolve:

```sh
./build/compiler_main path/to/program.c config/C
```

On Windows:

```powershell
.\build\compiler_main.exe path\to\program.c config\C
```

Arguments:

1. Source file to read.
2. Configuration directory containing `lex_config.json` and `parse_config.json`.

The CLI also declares `-o/--output`, `-w/--warn`, `-t/--trace`, and `-h/--help`. Declaring these options does not necessarily mean their associated functionality is implemented.

## Configuration

The included C-like language configuration is in `config/C/`.

- `lex_config.json` maps numeric token IDs to labels, regular-expression patterns, and skip flags.
- `parse_config.json` defines expression token roles, binding powers, reusable grammar variables, and statement rules.

See the documentation below for details.

## Documentation

- [Lexer](lexer.md)
- [JSON validator](json%20validator.md)
- [Grammar parser](parser/parser.md)
- [Pratt expression parser](parser/pratt_parser.md)

## Project scope

This repository currently implements a compiler frontend prototype.

Potential future stages include semantic analysis, an intermediate representation, optimization, and a backend.

These are future directions, not existing features.

The source code and checked-in configuration files should be treated as the source of truth when older documentation differs from the implementation.
