mkdir -p {obj,bin}/self

PATTERN="vector|architecture|ast|sym|type|parser.*|analyzer.*|emittEer.*|eval|compiler|options|main"

set -e

pushd obj/self >/dev/null
HOSTING=`find ../../src/*.c | egrep "$PATTERN"`
../../bin/$CONFIG/fcc -I ../../tests/include -c $HOSTING
popd >/dev/null

EXT=`find src/*.c | egrep -v "$PATTERN"`
gcc -m32 $CFLAGS $EXT obj/self/*.o -o bin/self/fcc
rm -f src/*.o