#include "../inc/hashmap.h"

#include "stdlib.h"
#include "string.h"
#include "assert.h"

using "../inc/hashmap.h";

using "stdlib.h";
using "stdio.h";
using "string.h";

static intptr_t hashstr (const char* key, int mapsize);
static intptr_t hashint (intptr_t element, int mapsize);

typedef void (*generalmapKeyDtor)(char* key, const void* value);
typedef void (*generalmapValueDtor)(void* value);
typedef intptr_t (*generalmapHash)(const char* key, int mapsize);
//Like strcmp, returns 0 for match
typedef int (*generalmapCmp)(const char* actual, const char* key);
typedef char* (*generalmapDup)(const char* key);

static generalmap* generalmapInit (generalmap* map, int size, bool hashes);

static void generalmapFree (generalmap* map, bool hashes);
static void generalmapFreeObjs (generalmap* map, generalmapKeyDtor keyDtor, generalmapValueDtor valueDtor,
                                bool hashes);

static bool generalmapAdd (generalmap* map, const char* key, void* value,
                           generalmapHash hashf, generalmapCmp cmp, bool values);
static void generalmapMerge (generalmap* dest, const generalmap* src,
                             generalmapHash hash, generalmapCmp cmp, generalmapDup dup, bool values);

static void* generalmapMap (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp);
static bool generalmapTest (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp);

/*:::: HASH FUNCTIONS ::::*/

static intptr_t hashstr (const char* key, int mapsize) {
    /*Jenkin's One-at-a-Time Hash
      Taken from http://www.burtleburtle.net/bob/hash/doobs.html
      Public domain*/

    intptr_t hash = 0;

    for (int i = 0; key[i]; i++) {
        hash += key[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    /*Assumes mapsize is a power of two*/
    intptr_t mask = mapsize-1;
    return hash & mask;
}

static intptr_t hashint (intptr_t element, int mapsize) {
    /*The above, for a single value*/

    intptr_t hash = element;
    hash += hash << 10;
    hash ^= hash >> 6;
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    intptr_t mask = mapsize-1;
    return hash & mask;
}

/*:::: GENERALMAP ::::*/

static bool generalmapIsMatch (const generalmap* map, int index, const char* key, int hash, generalmapCmp cmp);

/*Slightly counterintuitively, this doesn't actually find the location
  of the key. It finds the location where it stopped looking, which
  will be:
    1. The key
    2. The first zero after its expected position
    3. Another key, indicating that it was not found and that the
       buffer is full
  It is guaranteed to be a valid index in the buffer.*/
static int generalmapFind (const generalmap* map, const char* key,
                           int hash, generalmapCmp cmp);

static int pow2ize (int x) {
    assert(sizeof(x) <= 8);
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    /*No-op if x is less than 33 bits
      Shift twice to avoid warnings, as this is intended.*/
    x |= (x >> 16) >> 16;
    return x+1;
}

static generalmap* generalmapInit (generalmap* map, int size, bool hashes) {
    /*The hash requires that the size is a power of two*/
    size = pow2ize(size);

    map->size = size;
    map->elements = 0;
    map->keysInt = calloc(size, sizeof(char*));
    map->hashes = hashes ? calloc(size, sizeof(int)) : 0;
    map->values = calloc(size, sizeof(char*));

    return map;
}

static void generalmapFree (generalmap* map, bool hashes) {
    free(map->keysInt);

    if (hashes)
        free(map->hashes);

    free(map->values);

    map->keysInt = 0;
    map->hashes = 0;
    map->values = 0;
}

static void generalmapFreeObjs (generalmap* map, generalmapKeyDtor keyDtor, generalmapValueDtor valueDtor,
                                bool hashes) {
    /*Until the end of the buffer*/
    for (int index = 0; index < map->size; index++) {
        /*Skip empties*/
        if (map->values[index] == 0)
            continue;

        /*Call the dtor*/

        if (keyDtor)
            keyDtor(map->keysStrMutable[index], map->values[index]);

        if (valueDtor)
            valueDtor(map->values[index]);
    }

    generalmapFree(map, hashes);
}

static bool generalmapIsMatch (const generalmap* map, int index, const char* key, int hash, generalmapCmp cmp) {
    if (cmp)
        return    map->hashes[index] == hash
               && !cmp(map->keysStr[index], key);

    else
        return map->keysStr[index] == key;
}

static int generalmapFind (const generalmap* map, const char* key,
                           int hash, generalmapCmp cmp) {
    /*Check from the first choice slot all the way until the end of
      the values buffer, if need be*/
    for (int index = hash; index < map->size; index++)
        /*Exit if empty or found the key*/
        if (map->values[index] == 0 || generalmapIsMatch(map, index, key, hash, cmp))
            return index;

    /*Then from the start all the way back to the first choice slot*/
    for (int index = 0; index < hash; index++)
        if (map->values[index] == 0 || generalmapIsMatch(map, index, key, hash, cmp))
            return index;

    /*Done an entire sweep of the buffer, not found & full*/
    return hash;
}

static bool generalmapAdd (generalmap* map, const char* key, void* value,
                           generalmapHash hashf, generalmapCmp cmp, bool values) {
    /*Half full: create a new one twice the size and copy elements over.
      Allows us to assume there is space for the key.*/
    if (map->elements*2 + 1 >= map->size) {
        generalmap newmap;
        generalmapInit(&newmap, map->size*2, cmp != 0);
        generalmapMerge(&newmap, map, hashf, cmp, 0, values);
        generalmapFree(map, cmp != 0);
        *map = newmap;
    }

    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);

    bool present;

    /*Empty spot*/
    if (map->values[index] == 0) {
        map->keysStr[index] = key;
        map->elements++;
        present = false;

        if (cmp != 0)
            map->hashes[index] = hash;

    /*Present, remap*/
    } else {
        assert(generalmapIsMatch(map, index, key, hash, cmp));
        present = true;
    }

    map->values[index] = values ? value : (void*) true;

    return present;
}

static void generalmapMerge (generalmap* dest, const generalmap* src,
                             generalmapHash hash, generalmapCmp cmp, generalmapDup dup, bool values) {
    for (int index = 0; index < src->size; index++) {
        if (src->keysInt[index] == 0)
            continue;

        char* key = src->keysStrMutable[index];

        if (dup)
            key = dup(key);

        generalmapAdd(dest, key, values ? src->values[index] : 0, hash, cmp, values);
    }
}

static void* generalmapMap (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp) {
    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);
    return generalmapIsMatch(map, index, key, hash, cmp) ? map->values[index] : 0;
}

static bool generalmapTest (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp) {
    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);
    return generalmapIsMatch(map, index, key, hash, cmp);
}

/*:::: HASHMAP ::::*/

hashmap* hashmapInit (hashmap* map, int size) {
    return generalmapInit(map, size, true);
}

void hashmapFree (hashmap* map) {
    generalmapFree(map, true);
}

void hashmapFreeObjs (hashmap* map, hashmapKeyDtor keyDtor, hashmapValueDtor valueDtor) {
    generalmapFreeObjs(map, keyDtor, valueDtor, true);
}

bool hashmapAdd (hashmap* map, const char* key, void* value) {
    return generalmapAdd(map, key, value, hashstr, strcmp, true);
}

void hashmapMerge (hashmap* dest, hashmap* src) {
    generalmapMerge(dest, src, hashstr, strcmp, 0, true);
}

void hashmapMergeDup (hashmap* dest, const hashmap* src) {
    generalmapMerge(dest, src, hashstr, strcmp, strdup, true);
}

void* hashmapMap (const hashmap* map, const char* key) {
    return generalmapMap(map, key, hashstr, strcmp);
}

/*:::: INTMAP ::::*/

intmap* intmapInit (intmap* map, int size) {
    return generalmapInit(map, size, false);
}

void intmapFree (intmap* map) {
    generalmapFree(map, false);
}

void intmapFreeObjs (intmap* map, intmapValueDtor dtor) {
    generalmapFreeObjs(map, 0, (generalmapValueDtor) dtor, false);
}

bool intmapAdd (intmap* map, intptr_t element, void* value) {
    return generalmapAdd(map, (void*) element, value, (generalmapHash) hashint, 0, true);
}

void intmapMerge (intmap* dest, const intmap* src) {
    generalmapMerge(dest, src, (generalmapHash) hashint, 0, 0, true);
}

void* intmapMap (const intmap* map, intptr_t element) {
    return generalmapMap(map, (void*) element, (generalmapHash) hashint, 0);
}

/*:::: HASHSET ::::*/

hashset* hashsetInit (hashset* set, int size) {
    return generalmapInit(set, size, true);
}

void hashsetFree (hashset* set) {
    generalmapFree(set, true);
}

void hashsetFreeObjs (hashset* set, hashsetDtor dtor) {
    generalmapFreeObjs(set, (generalmapKeyDtor) dtor, 0, true);
}

bool hashsetAdd (hashset* set, const char* element) {
    return generalmapAdd(set, element, 0, hashstr, strcmp, false);
}

void hashsetMerge (hashset* dest, hashset* src) {
    generalmapMerge(dest, src, hashstr, strcmp, 0, false);
}

void hashsetMergeDup (hashset* dest, const hashset* src) {
    generalmapMerge(dest, src, hashstr, strcmp, strdup, false);
}

bool hashsetTest (const hashset* set, const char* element) {
    return generalmapTest(set, element, hashstr, strcmp);
}


/*:::: INTSET ::::*/

intset* intsetInit (intset* set, int size) {
    return generalmapInit(set, size, false);
}

void intsetFree (intset* set) {
    generalmapFree(set, false);
}

bool intsetAdd (intset* set, intptr_t element) {
    return generalmapAdd(set, (void*) element, 0, (generalmapHash) hashint, 0, false);
}

void intsetMerge (intset* dest, const intset* src) {
    generalmapMerge(dest, src, (generalmapHash) hashint, 0, 0, false);
}

bool intsetTest (const intset* set, intptr_t element) {
    return generalmapTest(set, (void*) element, (generalmapHash) hashint, 0);
}
