#include "../inc/vector.h"

#include "../std/std.h"

#include "stdlib.h"
#include "stdio.h"

vector vectorCreate (int initialCapacity) {
    vector v;
    v.length = 0;
    v.capacity = initialCapacity;
    v.buffer = malloc(initialCapacity*sizeof(void*));
    return v;
}

void vectorDestroy (vector* v) {
    free(v->buffer);
    v->buffer = 0;
}

void vectorDestroyObjs (vector* v, dtorType dtor) {
    /*This will mess up the vector, watevs*/
    vectorMap(v, (mapType) dtor, v);
    vectorDestroy(v);
}

void vectorAdd (vector* v, void* item) {
    if (v->length == v->capacity)
        v->buffer = realloc(v->buffer, (v->capacity *= 2)*sizeof(void*));

    v->buffer[v->length++] = item;
}

void* vectorGet (const vector* v, int n) {
    if (n < v->length)
        return v->buffer[n];

    else
        return 0;
}

int vectorSet (vector* v, int n, void* value) {
    if (n < v->length) {
        v->buffer[n] = value;
        return false;

    } else
        return true;
}

void vectorMap (vector* dest, mapType f, vector* src) {
    for (int n = 0; n < src->length; n++)
        dest->buffer[n] = f(src->buffer[n]);
}
