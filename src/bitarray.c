#include "../inc/bitarray.h"

#include "stdlib.h"

using "../inc/bitarray.h";

using "stdlib.h";

enum {
    bitsPerWord = 8*sizeof(bitarrayWord)
};

bitarray* bitarrayInit (bitarray* bits, int bitno) {
    /*Round up to fit enough bits*/
    int size = (bitno-1)/bitsPerWord + 1;
    bits->array = calloc(size, sizeof(bitarrayWord));
    bits->bitno = bitno;
    return bits;
}

void bitarrayFree (bitarray* bits) {
    free(bits->array);

    bits->array = 0;
    bits->bitno = 0;
}

bool bitarrayModify (bitarray* bits, int index, bool set) {
    if (index >= bits->bitno)
        return false;

    int word = index / bitsPerWord,
        bit = index % bitsPerWord;

    if (set)
        bits->array[word] |= 1 << bit;

    else
        bits->array[word] &= ~(1 << bit);

    return true;
}

bool bitarraySet (bitarray* bits, int index) {
    return bitarrayModify(bits, index, true);
}

bool bitarrayUnset (bitarray* bits, int index) {
    return bitarrayModify(bits, index, false);
}

bitarrayWord bitarrayTest (const bitarray* bits, int index) {
    if (index >= bits->bitno)
        return 0;

    int word = index / bitsPerWord,
        bit = index % bitsPerWord;

    return bits->array[word] & (1 << bit);
}
