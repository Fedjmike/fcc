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

void vectorFree (vector* v) {
    free(v->buffer);
    v->length = 0;
    v->capacity = 0;
    v->buffer = 0;
}

void vectorFreeObjs (vector* v, vectorDtor dtor) {
    /*This will mess up the vector, watevs*/
    vectorMap(v, (vectorMapper) dtor, v);
    vectorFree(v);
}

static void vectorResize (vector* v, int size) {
    v->capacity = size;
    v->buffer = realloc(v->buffer, v->capacity*sizeof(void*));
}

int vectorPush (vector* v, void* item) {
    if (v->length == v->capacity)
        vectorResize(v, v->capacity*2);

    v->buffer[v->length] = item;
    return v->length++;
}

vector* vectorPushFromArray (vector* v, void** array, int length, int elementSize) {
    /*Make sure it is at least big enough*/
    if (v->capacity < v->length + length)
        vectorResize(v, v->capacity + length*2);

    /*If the src and dest element size match, memcpy*/
    if (elementSize == sizeof(void*))
        memcpy(v->buffer+v->length, array, length*elementSize);

    /*Otherwise copy each element individually*/
    else
        for (int i = 0; i < length; i++)
            memcpy(v->buffer+i, (char*) array + i*elementSize, elementSize);

    v->length += length;
    return v;
}

vector* vectorPushFromVector (vector* dest, const vector* src) {
    return vectorPushFromArray(dest, src->buffer, src->length, sizeof(void*));
}

void* vectorPop (vector* v) {
    return v->buffer[--v->length];
}

int vectorFind (vector* v, void* item) {
    for (int i = 0; i < v->length; i++) {
        if (v->buffer[i] == item)
            return i;
    }

    return -1;
}

void* vectorRemoveReorder (vector* v, int n) {
    if (v->length <= n)
        return 0;

    void* last = vectorPop(v);
    vectorSet(v, n, last);
    return last;
}

void* vectorGet (const vector* v, int n) {
    if (n < v->length && n >= 0)
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

    dest->length = upto;
}
