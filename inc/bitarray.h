#pragma once

#include "stdint.h"

typedef intmax_t bitarrayWord;

/**
 * A packed array of bits aka bitmap, bitset, bitfield etc
 */
typedef struct bitarray {
    bitarrayWord* array;
    int bitno;
} bitarray;

bitarray* bitarrayInit (bitarray* bits, int bitno);
void bitarrayFree (bitarray* bits);

bool bitarrayModify (bitarray* bits, int index, bool set);
bool bitarraySet (bitarray* bits, int index);
bool bitarrayUnset (bitarray* bits, int index);

/**
 * Return non-zero if the bit was set
 */
bitarrayWord bitarrayTest (const bitarray* bits, int index);
