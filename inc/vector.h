typedef struct {
    int length, capacity;
    void** buffer;
} vector;

typedef void (*dtorType)(void*); ///For use with vectorDestroyObjs
typedef void* (*mapType)(void*); ///For use with vectorMap

vector vectorCreate (int initialCapacity);
void vectorDestroy (vector* v);
void vectorDestroyObjs (vector* v, void (*dtor)(void*));

void vectorAdd (vector* v, void* item);

/**
 * Maps dest[n] to f(src[n]) for n in src->length.
 *
 * src and dest can match. Lengths not checked.
 * @see mapType
 */
void vectorMap (vector* dest, vector* src, void* (*f)(void*));
