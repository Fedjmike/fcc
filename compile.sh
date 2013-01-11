#!/bin/sh
set -e
bin/debug/cc.exe $1.c $1.asm
assemble.sh $1
