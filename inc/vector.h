#pragma once

typedef struct vector {
    int length, capacity;
    void** buffer;
} vector;

typedef void (*vectorDtor)(void*); ///For use with vectorDestroyObjs
typedef void* (*vectorMapper)(void*); ///For use with vectorMap

vector vectorCreate (int initialCapacity);

/**
 * Cleans up resources allocated by the vector but not the vector itself.
 */
void vectorDestroy (vector* v);
void vectorDestroyObjs (vector* v, void (*dtor)(void*));

void vectorPush (vector* v, void* item);
void* vectorPop (vector* v);

/**
 * Returns the value, or null if out of bounds.
 */
void* vectorGet (const vector* v, int n);

/**
 * Attempts to set an index to a value. Returns non-zero on failure.
 */
int vectorSet (vector* v, int n, void* value);

/**
 * Maps dest[n] to f(src[n]) for n in src->length.
 *
 * src and dest can match. Lengths not checked.
 * @see mapType
 */
void vectorMap (vector* dest, void* (*f)(void*), vector* src);
