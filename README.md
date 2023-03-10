# Slime SysY Compiler

SSYC is an ARM-v7a assembly compiler for SysY written in C++20 and YACC.

# Build

```sh
cmake -B build -S . --install-prefix <install-prefix>
cmake --build build --target install
<install-prefix>/bin/SSYC [options]
```

# Usage

```sh
SSYC [--input <source_file>]
```

Options:
- `--help`: show usage
- `--input`: specify the source file (stdin by default)
