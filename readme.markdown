Fedjmike's C Compiler
=====================

Features
--------

The compiler implements a language quite similar to C, but there are some major differences. The following list is probably not exhaustive, but attempts to be.

- Addition of:
  - Simple module system
  - `bool` type
- Different semantics:
  - Unified variable/typedef/struct/union/enum namespace
  - Logical operators (`||`, `&&`, and `!`) return `bool`
  - Empty prototype parentheses means zero arguments
  - Operator precedence simplified
  - Ternary (`?:`) can return lvalues (as in C++)
  - Comma (`,`) can return a constant expression
  - Enums are always complete types
- The features of C99 and C11 supported:
  - Anonymous structs/unions
  - Compound literals
  - Intermingled code/declarations including for-loop declarations
  - C++ comments
- No support for / features removed:
  - Preprocessor
  - `switch`
  - Floating point types
  - Unsigned integers
  - `long`
  - Wide characters
  - Bitfields
  - Implicit casts / coercion between integral types
  - `goto` and labels
  - Designated initializers for initializers and compound literals
  - `volatile`
  - `register` storage class

The compiler is advanced enough to self-host much of itself with the rest compiled with GCC. As the compiler matures experimental additions to the language considered are:

- Type polymorphism as in ML/Haskell
- Lambdas/closures
- Option types (`option` in ML, `Maybe` in Haskell)
- Algebraic types (a more general solution to option types)

Output
------

The compiler generates assembly for x86 (32-bit) CPUs. 64-bit AMD64 is experimental. The compiler does very little optimization before emitting straight to assembly.

The ABI is largely compatible GCC and Clang's. However, passing and returning structs as values will often not work. This means that most code compiled with one compiler can be successfully linked and run with code compiled with another.

Building
--------

Building the compiler is simple, just call make in the root directory:

```bash
cd <fcc>
make CONFIG=release
```

This puts an fcc binary in `<fcc>/bin/$CONFIG`

Makefile options (all optional):
- `OS=[linux windows]`, default: `linux`
- `ARCH=[32 64]`, default: `32`
- `CONFIG=[debug profiling release]`, default: `release`
- `CC=[path to C11 compiler]`, default: `gcc`
- `FCC=[path to FCC binary]`, default: `bin/$CONFIG/fcc`
- `VALGRIND`, default: true (if Valgrind is found)

Makefile targets:
- `all clean print  tests-all print-tests  selfbuild`

Use the `FCC` option to choose which copy of FCC to run the tests against.

The `selfbuild` target executes a partial self-host: a predefined set of modules is compiled by FCC, the rest by GCC (this is not overrideable). The result goes in `bin/self/fcc`. As the language accepted by the compiler differs slightly to C, only the `selfhosting` branch will compile under both. [The modifications](https://github.com/Fedjmike/fcc/compare/selfhosting) required to be a [polyglot](http://en.wikipedia.org/wiki/Polyglot_(computing)) are quite minimal, mostly just concerning the module system and explicit type coercion.

If found on the system the compiler will be run against Valgrind when building tests. To disable this, set `VALGRIND=""`.

Simply specifying `FCC=bin/self/fcc` as an option for the `tests-all` target will trigger a partial self-host.

Running
-------

The command line interface is similar to that of GCC:

Usage: `fcc [--help] [--version] [-csS] [-I <dir>] [-o <file>] <files...>`
- `-I <dir>`   Add a directory to be searched for headers
- `-c`         Compile and assemble only, do not link
- `-S`         Compile only, do not assemble or link
- `-s`         Keep temporary assembly output after compilation
- `-o <file>`  Output into a specific file
- `--help`     Display command line information
- `--version`  Display version information
