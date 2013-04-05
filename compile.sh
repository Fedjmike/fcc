#!/bin/sh
set -e
bin/debug/cc.exe $1.c
assemble.sh $1
