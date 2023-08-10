<div align="center">

# SSYC

SSYC is an ARM-v7a assembly compiler for SysY written in pure C++17.

[Learn about SSYC](#learn-about-ssyc) •
[Installation](#installation) •
[Getting started](#getting-started)

</div>

# Learn about SSYC

SSYC, full name slime SysY compiler, is a compiler for C-like language SysY.

## Features

- Ease to use compiler driver
- Hand written frontend
- Strong typed IR in SSA form similar to LLVM-IR
- Minimal standard library for IO, debug and profiling
- Pretty assembly output

# Installation

## Requirement

Minimum requirements

- CMake >= 3.5
- C++ compiler supporting C++17 standard

Recommended configuration (for test, profiling and use of scripts)

- Git
- Clang >= 15.0.0
- 32-bit standard libraries for C/C++
- GTest
- nushell >= 0.78.0
- fd >= 8.5.3
- ripgrep >= 13.0.0

## Build compiler

```sh
git clone https://github.com/zymelaii/SSYC.git
cd SSYC
cmake -B build -S . --install-prefix <install-prefix>
cmake --build build --target install
```

## Generate testcase program

```sh
cmake -B build -S . -DBUILD_TEST=ON
cmake --build build --target testcase
```

# Getting started

## Usage

```sh
$ slimec -h
ARM-v7a assembly compiler for SysY wriiten in C++ 17

Usage: slimec [options] file...

Options:
  -h, --help           Print help information
  -v, --version        Print version information
  --                   Read from stdin
  -E                   Only run the preprocessor
  --lex-only           Only run the lexer
  -S                   Only run preprocess and compilation steps
  --dump-ast           Use the AST representation for output
  --emit-ir            Use the MIR representation for assembler
  -o, --output <file>  Write output to <file>
  -p, --pipe           Always output to stdout
  -O<opt-level>        Specify optimize level [UNCOMPLETE]
  -f<flag>             Provide extra flag [UNCOMPLETE]
```
