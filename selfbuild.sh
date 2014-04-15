mkdir -p {obj,bin}/self

PATTERN="vector|hashmap|architecture|ast|sym|type|operand|asm-amd64|parser.*|analyzer.*|emitter.*|eval|compiler|options|main"

set -e

pushd obj/self >/dev/null
HOSTING=`find ../../src/*.c | egrep "$PATTERN"`
valgrind -q ../../bin/$CONFIG/fcc -I ../../tests/include -c $HOSTING
popd >/dev/null

EXT=`find src/*.c | egrep -v "$PATTERN"`
gcc -g -m32 $CFLAGS $EXT obj/self/*.o -o bin/self/fcc
rm -f src/*.o

HOSTING=`find src/*.c | egrep "$PATTERN"`
HNO=`cat $HOSTING | wc -l`
ENO=`cat $EXT | wc -l`
CALC=$HNO/\($ENO+$HNO\)*100
echo $HNO / `bc -l <<< $ENO+$HNO` = `bc -l <<< $CALC`