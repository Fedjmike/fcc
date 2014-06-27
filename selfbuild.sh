mkdir -p {obj,bin}/self

ANTIPATTERN="reg|lexer\.c"
ANTIANTIPATTERN="reg"

set -e

cd src
HOSTING=`find *.c | egrep -v "$ANTIPATTERN"`
echo $HOSTING
cd ..

pushd obj/self >/dev/null
HOSTING=`find ../../src/*.c | egrep -v "$ANTIPATTERN"`
../../bin/$CONFIG/fcc -I ../../tests/include -c $HOSTING
popd >/dev/null

EXT=`find src/*.c | egrep "$ANTIANTIPATTERN"`
gcc -g -m32 $CFLAGS $EXT obj/self/*.o -o bin/self/fcc
rm -f src/*.o

HOSTING=`find src/*.c | egrep -v "$ANTIPATTERN"`
HNO=`cat $HOSTING | wc -l`
ENO=`cat $EXT | wc -l`
echo $EXT
CALC=$HNO/\($ENO+$HNO\)*100
echo $HNO / `bc -l <<< $ENO+$HNO` = `bc -l <<< $CALC`