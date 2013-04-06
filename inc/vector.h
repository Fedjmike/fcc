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

void vectorMap (void* (*f)(void*), vector* v);
