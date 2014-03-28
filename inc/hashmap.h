#pragma once

#include "../std/std.h"

#include "stdint.h"

/**
 * This header provides 4 different data structures.
 *
 * Abstractly, a map translates unique keys to arbitrary values. A set contains
 * unique elements, that is to say any possible member either belongs to it or
 * doesn't. Neither has any concept of an ordering.
 *
 * Details of concrete implementations:
 *  - hashmap is a map from null terminated (char*) strings to void* values.
      It is the most general of these structures.
 *  - intmap is a map from uintptr_t integers to void* values.
 *  - hashset is a set of strings.
 *  - intset is a set of uintptr_t integers.
 *
 * They are implemented by the same algorithm and struct, generalmap. Use the
 * specific interfaces, and avoid using the struct fields directly.
 *
 * Important notes on pointer ownership / cleanup:
 *  - hashXXXFree does not free the string keys given to it.
 *  - XXXmapFree does not free the void* values given to it.
 *  - XXXFreeObjs can be used to explicitly free these by providing destructor(s).
 * and
 *  - hashXXXMerge uses the same string keys in the destination as in the source.
 *  - Use hashXXXMergeDup to instead make a copy of each added to the dest.
 *  - Both will use the same void* value.
 */

/**
 * An efficient data structure for mapping from null terminated keys
 * to void* values.
 */
typedef struct generalmap {
    int size, elements;
    union {
        const char** keysStr;
        /*We don't know whether the user intends the map to take ownership of
          keys (freed with FreeObjs), so provide a mutable version*/
        char** keysStrMutable;
        intptr_t* keysInt;
    };
	int* hashes;
    void** values;
} generalmap;

typedef generalmap hashmap;
typedef generalmap intmap;

typedef generalmap hashset;
typedef generalmap intset;

/*:::: HASHMAP ::::*/

typedef void (*hashmapKeyDtor)(char* key, const void* value);
typedef void (*hashmapValueDtor)(void* value);

hashmap* hashmapInit (hashmap* map, int size);

void hashmapFree (hashmap* map);
void hashmapFreeObjs (hashmap* map, hashmapKeyDtor keyDtor, hashmapValueDtor valueDtor);

bool hashmapAdd (hashmap* map, const char* key, void* value);
void hashmapMerge (hashmap* dest, hashmap* src);
void hashmapMergeDup (hashmap* dest, const hashmap* src);

void* hashmapMap (const hashmap* map, const char* key);

/*:::: INTMAP ::::*/

typedef void (*intmapValueDtor)(void* value, int key);

intmap* intmapInit (intmap* map, int size);

void intmapFree (intmap* map);
void intmapFreeObjs (intmap* map, intmapValueDtor dtor);

bool intmapAdd (intmap* map, intptr_t element, void* value);
void intmapMerge (intmap* dest, const intmap* src);

void* intmapMap (const intmap* map, intptr_t element);

/*:::: HASHSET ::::*/

typedef void (*hashsetDtor)(char* element);

hashset* hashsetInit (hashset* set, int size);

void hashsetFree (hashset* set);
void hashsetFreeObjs (hashset* set, hashsetDtor dtor);

bool hashsetAdd (hashset* set, const char* element);
void hashsetMerge (hashset* dest, hashset* src);
void hashsetMergeDup (hashset* dest, const hashset* src);

bool hashsetTest (const hashset* set, const char* element);

/*:::: INTSET ::::*/

intset* intsetInit (intset* set, int size);
void intsetFree (intset* set);

bool intsetAdd (intset* set, intptr_t element);
void intsetMerge (intset* dest, const intset* src);

bool intsetTest (const intset* set, intptr_t element);
