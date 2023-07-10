# Slime SysY Compiler

SSYC is an ARM-v7a assembly compiler for SysY written in pure C++17.

# Build

```sh
cmake -B build -S . --install-prefix <install-prefix>
cmake --build build --target install
<install-prefix>/bin/slimec [options]
```

# Usage

```sh
slimec [<source>] [options]
```

Options:

- `--lex-only` perfom lex only
- `--dump-ast` dump AST to stderr
