set -e
set -v

gcc -std=c11 $CFLAGS -c *.c # -S -O3
../bin/$1/fcc -I ../tests/include -c ast.c sym.c parser.c
gcc *.o -o fcc
rm *.o

fcc -I ../tests/include ../tests/xor-list.c -s
../tests/xor-list.exe
