#include "../inc/vector.h"

#include "../std/std.h"

#include "stdlib.h"
#include "string.h"

vector* vectorInit (vector* v, int initialCapacity) {
    v->length = 0;
    v->capacity = initialCapacity;
    v->buffer = malloc(initialCapacity*sizeof(void*));
    return v;
}

vector* vectorInitFromArray (vector* v, void** array, int length, int initialCapacity) {
    vectorInit(v, initialCapacity);
    memcpy(v->buffer, array, length*sizeof(void*));
    v->length = length;
    return v;
}

void vectorFree (vector* v) {
    free(v->buffer);
    v->buffer = 0;
}

void vectorFreeObjs (vector* v, vectorDtor dtor) {
    /*This will mess up the vector, watevs*/
    vectorMap(v, (vectorMapper) dtor, v);
    vectorFree(v);
}

void vectorPush (vector* v, void* item) {
    if (v->length == v->capacity)
        v->buffer = realloc(v->buffer, (v->capacity *= 2)*sizeof(void*));

    v->buffer[v->length++] = item;
}

void* vectorPop (vector* v) {
    return v->buffer[v->length -= 1];
}

void* vectorGet (const vector* v, int n) {
    if (n < v->length)
        return v->buffer[n];

    else
        return 0;
}

bool vectorSet (vector* v, int n, void* value) {
    if (n < v->length) {
        v->buffer[n] = value;
        return false;

    } else
        return true;
}

void vectorMap (vector* dest, vectorMapper f, vector* src) {
    int upto = src->length > dest->capacity ? dest->capacity : src->length;

    for (int n = 0; n < upto; n++)
        dest->buffer[n] = f(src->buffer[n]);
}
