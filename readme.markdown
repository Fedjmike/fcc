Fedjmike's C Compiler
=====================

License
-------

Unless otherwise stated, a source file in this package is under the GNU GPL V3 license.

Fedjmike's C Compiler Copyright (C) 2012-2014 Sam Nipps

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. 

You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.

Building
--------

Building the compiler is simple, just call make in the root directory:

```bash
cd <fcc>
make CONFIG=release
```

Makefile options (all optional):
- `CONFIG=[debug profiling release]`, default: `debug`
- `CC=[path to C11 compiler]`, default: `gcc`

Makefile targets:
- `all clean print`

This puts an fcc binary in `<fcc>/bin/$CONFIG`

Running
-------

The command line interface is similar to that of GCC:

Usage: `fcc [--version] [--help] [-csS] [-I <dir>] [-o <file>] <files...>`
- `-c`         Compile and assemble only, do not link
- `--help`     Display command line information
- `-I <dir>`   Add a directory to be searched for headers
- `-o <file>`  Output into a specific file
- `-s`         Keep temporary assembly output after compilation
- `-S`         Compile only, do not assemble or link
- `--version`  Display version information
