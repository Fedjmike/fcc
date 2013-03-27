#!/bin/sh
set -e
bin/debug/as.exe $1.asm -o $1.o
bin/debug/ld.exe $1.o -e _main -o $1.exe
rm $1.o
echo
set +e
$1.exe
echo EXECUTING: Returned $?