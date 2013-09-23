#include "../inc/hashmap.h"

#include "stdlib.h"
#include "string.h"

static int hashmapHash (const char* key, int mapsize);

/*Slightly counterintuitively, this doesn't actually find the location
  of the key. It finds the location where it stopped looking, which
  will be:
    1. The key
    2. The first zero after its expected position
    3. Another key, indicating that it was not found and that the
       buffer is full
  It is guaranteed to be a valid index in the buffer.*/
static int hashmapFind (const hashmap* map, const char* key);

hashmap hashmapCreate (int size) {
    return (hashmap) {size, 0, calloc(size, sizeof(char*)), calloc(size, sizeof(char*))};
}

void hashmapDestroy (hashmap* map) {
    for (int index = 0; index < map->size; index++) {
        while (map->keys[index] == 0)
            index++;

        free(map->keys[index]);
    }

    free(map->keys);
    free(map->values);
}

void hashmapDestroyObjs (hashmap* map, hashmapDtor dtor) {
    for (int index = 0; index < map->size; index++) {
        while (map->keys[index] == 0)
            index++;

        dtor(map->keys[index], map->values[index]);
        free(map->keys[index]);
    }

    free(map->keys);
    free(map->values);
}

static int hashmapHash (const char* key, int mapsize) {
    int hash = 0;

    for (int i = 0; key[i]; i++)
        hash += (int) key[i];

    return hash % mapsize;
}

static int hashmapFind (const hashmap* map, const char* key) {
    int hash = hashmapHash(key, map->size);

    /*Check from the first choice slot all the way until the end of
      the values buffer, if need be*/
    for (int index = hash; index < map->size; index++)
        if (map->keys[index] == 0 || !strcmp(map->keys[index], key))
            return index;

    /*Then from the start all the way back to the first choice slot*/
    for (int index = 0; index < hash; index++)
        if (map->keys[index] == 0 || !strcmp(map->keys[index], key))
            return index;

    /*Done an entire sweep of the buffer, not found & full*/
    return hash;
}

bool hashmapAdd (hashmap* map, const char* key, void* value) {
    int index = hashmapFind(map, key);

    /*Present, remap*/
    if (!strcmp(map->keys[index], key)) {
        map->values[index] = value;
        return true;

    /*Empty spot*/
    } else if (map->keys[index] == 0) {
        map->keys[index] = strdup(key);
        map->values[index] = value;
        map->elements++;
        return false;

    /*Full: create a new one twice the size and add it to that*/
    } else {
        hashmap newmap = hashmapCreate(map->size*2);
        hashmapMerge(&newmap, map);
        hashmapAdd(&newmap, key, value);

        hashmapDestroy(map);
        *map = newmap;
        return false;
    }
}

void hashmapMerge (hashmap* dest, const hashmap* src) {
    for (int index = 0; index < src->size; index++) {
        while (src->keys[index] == 0)
            index++;

        hashmapAdd(dest, src->keys[index], src->values[index]);
    }
}

void* hashmapMap (const hashmap* map, const char* key) {
    int index = hashmapFind(map, key);
    return !strcmp(map->values[index], key) ? map->values[index] : 0;
}
