Fedjmike's C Compiler
=====================

License
-------

Unless otherwise stated, a source file in this package is under the GNU GPL V3 license.

Fedjmike's C Compiler Copyright (C) 2012-2014 Sam Nipps

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. 

You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.

Features
--------

The compiler implements a language quite similar to C, but there are some major differences. The following list is probably not exhaustive, but attempts to be.

- Addition of:
  - Simple module system
  - `bool` type
- Different semantics:
  - Unified variable/typedef/struct/union/enum namespace
  - Logical operators (`||` and `&&`) return `bool`
  - Empty prototype parentheses means zero arguments
  - Operator precedence simplified
  - Ternary (`?:`) can return lvalues (as in C++)
  - Comma (`,`) can return a constant expression
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
  - Some numerical operators including divide (`/`) and modulo (`%`)
  - Designated initializers for initializers and compound literals
  - `volatile`
  - `register` storage class

The compiler is advanced enough to self-host much of itself with the rest compiled with GCC. As the compiler matures experimental additions to the language considered are:

- Type polymorphism as in ML/Haskell
- Lambdas/closures
- Options types (`option` in ML, `Maybe` in Haskell)
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
- `FCC=[path to FCC binary],` default: `bin/$CONFIG/fcc`

Makefile targets:
- `all clean print  tests-all print-tests  selfbuild`

Use the `FCC` option to choose which copy of FCC to run the tests against.

The `selfbuild` target executes a partial self-host: a predefined set of modules are compiled by FCC, the rest by GCC (this is not overrideable). The result goes in `bin/self/fcc`. As the language accepted by the compiler differs slightly to C, only the `selfhosting` branch will compile under both. [The modifications](https://github.com/Fedjmike/fcc/compare/selfhosting) required to be a [polyglot](http://en.wikipedia.org/wiki/Polyglot_(computing)) are quite minimal, mostly just concerning the module system and explicit type coercion.

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
