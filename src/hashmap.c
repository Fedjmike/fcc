#include "../inc/hashmap.h"

#include "stdlib.h"
#include "string.h"

static int hashstr (const char* key, int mapsize);

typedef void (*generalmapKeyDtor)(char* key, const void* value);
typedef void (*generalmapValueDtor)(void* value);
typedef int (*generalmapHash)(const char* key, int mapsize);
//Like strcmp, returns 0 for match
typedef int (*generalmapCmp)(const char* actual, const char* key);

static generalmap* generalmapInit (generalmap* map, int size, bool hashes, bool values);

static void generalmapFree (generalmap* map, bool hashes, bool values);
static void generalmapFreeObjs (generalmap* map, generalmapKeyDtor keyDtor, generalmapValueDtor valueDtor,
                                bool hashes, bool values);

static bool generalmapAdd (generalmap* map, const char* key, void* value,
                           generalmapHash hashf, generalmapCmp cmp, bool values);
static void generalmapMerge (generalmap* dest, const generalmap* src,
                             generalmapHash hash, generalmapCmp cmp, bool values, bool dup);

static void* generalmapMap (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp);
static bool generalmapTest (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp);

/*:::: HASH FUNCTIONS ::::*/

static int hashstr (const char* key, int mapsize) {
    int hash = 0;

    for (int i = 0; key[i]; i++)
        hash += key[i];

    return hash % mapsize;
}

static int hashint (int element, int mapsize) {
    return element % mapsize;
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

static generalmap* generalmapInit (generalmap* map, int size, bool hashes, bool values) {
    map->size = size;
    map->elements = 0;
    map->keysInt = calloc(size, sizeof(char*));
    map->hashes = hashes ? calloc(size, sizeof(int)) : 0;
    map->values = values ? calloc(size, sizeof(char*)) : 0;

    return map;
}

static void generalmapFree (generalmap* map, bool hashes, bool values) {
    free(map->keysInt);

    if (hashes)
        free(map->hashes);

    if (values)
        free(map->values);

    map->keysInt = 0;
    map->hashes = 0;
    map->values = 0;
}

static void generalmapFreeObjs (generalmap* map, generalmapKeyDtor keyDtor, generalmapValueDtor valueDtor,
                                bool hashes, bool values) {
    /*Until the end of the buffer*/
    for (int index = 0; index < map->size; index++) {
        /*Skip empties*/
        if (map->keysInt[index] == 0)
            continue;

        /*Call the dtor*/

        if (keyDtor)
            keyDtor(map->keysStrMutable[index], map->values[index]);

        if (valueDtor)
            valueDtor(map->values[index]);
    }

    generalmapFree(map, hashes, values);
}

static bool generalmapIsMatch (const generalmap* map, int index, const char* key, int hash, generalmapCmp cmp) {
    if (cmp)
        return    map->hashes[index] == hash
               && (!cmp || !cmp(map->keysStr[index], key));

    else
        return map->keysStr[index] == key;
}

static int generalmapFind (const generalmap* map, const char* key,
                           int hash, generalmapCmp cmp) {
    /*Check from the first choice slot all the way until the end of
      the values buffer, if need be*/
    for (int index = hash; index < map->size; index++)
        /*Exit if empty or found the key*/
        if (map->keysInt[index] == 0 || generalmapIsMatch(map, index, key, hash, cmp))
            return index;

    /*Then from the start all the way back to the first choice slot*/
    for (int index = 0; index < hash; index++)
        if (map->keysInt[index] == 0 || generalmapIsMatch(map, index, key, hash, cmp))
            return index;

    /*Done an entire sweep of the buffer, not found & full*/
    return hash;
}

static bool generalmapAdd (generalmap* map, const char* key, void* value,
                           generalmapHash hashf, generalmapCmp cmp, bool values) {
    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);

    /*Present, remap*/
    if (generalmapIsMatch(map, index, key, hash, cmp)) {
        map->keysStr[index] = key;

        if (cmp != 0)
            map->hashes[index] = hash;

        if (values)
            map->values[index] = value;

        return true;

    /*Empty spot*/
    } else if (map->keysInt[index] == 0) {
        map->keysStr[index] = key;
        map->elements++;

        if (cmp != 0)
            map->hashes[index] = hash;

        if (values)
            map->values[index] = value;

        return false;

    /*Full: create a new one twice the size and add it to that*/
    } else {
        generalmap newmap;
        generalmapInit(&newmap, map->size*2, cmp != 0, values);
        generalmapMerge(&newmap, map, hashf, cmp, values, false);
        generalmapAdd(&newmap, key, value, hashf, cmp, values);

        generalmapFree(map, values, cmp != 0);
        *map = newmap;
        return false;
    }
}

static void generalmapMerge (generalmap* dest, const generalmap* src,
                             generalmapHash hash, generalmapCmp cmp, bool values, bool dup) {
    for (int index = 0; index < src->size; index++) {
        if (src->keysInt[index] == 0)
            continue;

        char* key = src->keysStrMutable[index];

        if (dup)
            key = strdup(key);

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
    return generalmapInit(map, size, true, true);
}

void hashmapFree (hashmap* map) {
    generalmapFree(map, true, true);
}

void hashmapFreeObjs (hashmap* map, hashmapKeyDtor keyDtor, hashmapValueDtor valueDtor) {
    generalmapFreeObjs(map, keyDtor, valueDtor, true, true);
}

bool hashmapAdd (hashmap* map, const char* key, void* value) {
    return generalmapAdd(map, key, value, hashstr, strcmp, true);
}

void hashmapMerge (hashmap* dest, hashmap* src) {
    generalmapMerge(dest, src, hashstr, strcmp, true, false);
}

void hashmapMergeDup (hashmap* dest, const hashmap* src) {
    generalmapMerge(dest, src, hashstr, strcmp, true, true);
}

void* hashmapMap (const hashmap* map, const char* key) {
    return generalmapMap(map, key, hashstr, strcmp);
}

/*:::: INTMAP ::::*/

intmap* intmapInit (intmap* map, int size) {
    return generalmapInit(map, size, false, true);
}

void intmapFree (intmap* map) {
    generalmapFree(map, false, true);
}

void intmapFreeObjs (intmap* map, intmapValueDtor dtor) {
    generalmapFreeObjs(map, 0, (generalmapValueDtor) dtor, false, true);
}

bool intmapAdd (intmap* map, intptr_t element, void* value) {
    return generalmapAdd(map, (void*) element, value, (generalmapHash) hashint, 0, true);
}

void intmapMerge (intmap* dest, const intmap* src) {
    generalmapMerge(dest, src, (generalmapHash) hashint, 0, true, false);
}

void* intmapMap (const intmap* map, intptr_t element) {
    return generalmapMap(map, (void*) element, (generalmapHash) hashint, 0);
}

/*:::: HASHSET ::::*/

hashset* hashsetInit (hashset* set, int size) {
    return generalmapInit(set, size, true, false);
}

void hashsetFree (hashset* set) {
    generalmapFree(set, true, false);
}

void hashsetFreeObjs (hashset* set, hashsetDtor dtor) {
    generalmapFreeObjs(set, (generalmapKeyDtor) dtor, 0, true, false);
}

bool hashsetAdd (hashset* set, const char* element) {
    return generalmapAdd(set, element, 0, hashstr, strcmp, false);
}

void hashsetMerge (hashset* dest, hashset* src) {
    generalmapMerge(dest, src, hashstr, strcmp, false, false);
}

void hashsetMergeDup (hashset* dest, const hashset* src) {
    generalmapMerge(dest, src, hashstr, strcmp, false, true);
}

bool hashsetTest (const hashset* set, const char* element) {
    return generalmapTest(set, element, hashstr, strcmp);
}


/*:::: INTSET ::::*/

intset* intsetInit (intset* set, int size) {
    return generalmapInit(set, size, false, false);
}

void intsetFree (intset* set) {
    generalmapFree(set, false, false);
}

bool intsetAdd (intset* set, intptr_t element) {
    return generalmapAdd(set, (void*) element, 0, (generalmapHash) hashint, 0, false);
}

void intsetMerge (intset* dest, const intset* src) {
    generalmapMerge(dest, src, (generalmapHash) hashint, 0, false, false);
}

bool intsetTest (const intset* set, intptr_t element) {
    return generalmapTest(set, (void*) element, (generalmapHash) hashint, 0);
}
