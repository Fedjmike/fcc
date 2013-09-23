#include "../std/std.h"

/**
 * An efficient data structure for mapping from null terminated keys
 * to void* values.
 */
typedef struct hashmap {
    int size, elements;
    char** keys;
    void** values;
} hashmap;

typedef void (*hashmapDtor)(const char* key, void* value);

hashmap hashmapCreate (int size);
void hashmapDestroyObjs (hashmap* map, hashmapDtor dtor);

bool hashmapAdd (hashmap* map, const char* key, void* value);
void hashmapMerge (hashmap* dest, const hashmap* src);

void* hashmapMap (const hashmap* map, const char* key);
