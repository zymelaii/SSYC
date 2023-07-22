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
slimec [<source>] [options]
```

Options:

- `--lex-only` perform lex only
- `--dump-ast` dump AST to stderr
- `--emit-ir` create MIR code
- `-S` create assembler code
