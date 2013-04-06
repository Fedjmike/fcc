#include "../inc/vector.h"

#include "stdlib.h"

vector vectorCreate (int initialCapacity) {
    vector v;
    v.length = 0;
    v.capacity = initialCapacity;
    v.buffer = malloc(initialCapacity*sizeof(void*));
    return v;
}

void vectorDestroy (vector* v) {
    free(v->buffer);
    free(v);
}

void vectorDestroyObjs (vector* v, dtorType dtor) {
    /*This will mess up the vector, watevs*/
    vectorMap((mapType) dtor, v);
    vectorDestroy(v);
}

void vectorAdd (vector* v, void* item) {
    if (v->length == v->capacity)
        v->buffer = realloc(v->buffer, v->capacity *= 2);

    v->buffer[v->length++] = item;
}

void vectorMap (mapType f, vector* v) {
    for (int n = 0; n < v->length; n++)
        v->buffer[n] = f(v->buffer[n]);
}
