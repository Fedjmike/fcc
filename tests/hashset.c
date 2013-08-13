#include "compat.h"

void* malloc (int);
void* calloc (int, int);
void free (void*);

int printf (char*, ...);
int puts (char*);

void srand (int);
int rand ();

int time (void*);

typedef struct hashset {
    int size, elements;
    void** buffer;
} hashset;

typedef struct hashset_iter {
    hashset* set;
    int index;
} hashset_iter;

hashset hashset_create (int size);
void hashset_destroy (hashset* set);

bool hashset_add (hashset* set, void* element);
void hashset_add_all (hashset* dest, hashset* src);

bool hashset_test (hashset* set, void* element);

hashset_iter hashset_begin (hashset* set);
hashset_iter hashset_next (hashset_iter iter);
bool hashset_is_end (hashset_iter iter);
void* hashset_get (hashset_iter iter);

#include "stdlib.h"

static int hashset_hash (hashset* set, void* element);

/*Slightly counterintuitively, this doesn't actually find the location
  of the element. It finds the location where it stopped looking, which
  will be:
    1. The element
    2. The first zero after its expected position
    3. Another element, indicating that it was not found and that the
       buffer is full
  It is guaranteed to be a valid index in the buffer.*/
static int hashset_find (hashset* set, void* element);

hashset hashset_create (int size) {
    return (hashset) {size, 0, calloc(size, sizeof(void*))};
}
void hashset_destroy (hashset* set) {
    free(set->buffer);
}

static int hashset_hash (hashset* set, void* element) {
    return (int) element % set->size;
}

static int hashset_find (hashset* set, void* element) {
    int hash = hashset_hash(set, element);

    /*Check from the first choice slot all the way until the end of
      the buffer, if need be*/
    for (int index = hash; index < set->size; index++)
        if (set->buffer[index] == 0 || set->buffer[index] == element)
            return index;

    /*Then from the start all the way back to the first choice slot*/
    for (int index = 0; index < hash; index++)
        if (set->buffer[index] == 0 || set->buffer[index] == element)
            return index;

    /*Done an entire sweep of the buffer, not found & full*/
    return hash;
}

bool hashset_add (hashset* set, void* element) {
    int index = hashset_find(set, element);

    /*Present*/
    if (set->buffer[index] == element)
        return true;

    /*Empty spot*/
    else if (set->buffer[index] == 0) {
        set->buffer[index] = element;
        set->elements++;
        return false;

    /*Full: create a new one twice the size and add it to that*/
    } else {
        hashset newset = hashset_create(set->size*2);
        hashset_add_all(&newset, set);
        hashset_add(&newset, element);
        hashset_destroy(set);
        *set = newset;
        return false;
    }
}

void hashset_add_all (hashset* dest, hashset* src) {
    for (hashset_iter iter = hashset_begin(src);
         !hashset_is_end(iter);
         iter = hashset_next(iter))
        hashset_add(dest, hashset_get(iter));
}

bool hashset_test (hashset* set, void* element) {
    return set->buffer[hashset_find(set, element)] == element;
}

/*:::: Iterators ::::*/

hashset_iter hashset_begin (hashset* set) {
    return hashset_next((hashset_iter) {set, -1});
}

hashset_iter hashset_next (hashset_iter iter) {
    iter.index++;

    for (; iter.index < iter.set->size; iter.index++)
        if (iter.set->buffer[iter.index] != 0)
            return iter;

    return (hashset_iter) {iter.set, -1};
}

bool hashset_is_end (hashset_iter iter) {
    return iter.index == -1;
}

void* hashset_get (hashset_iter iter) {
    return iter.set->buffer[iter.index];
}

#include "stdlib.h"
#include "time.h"
#include "stdio.h"

int main (int argc, char** argv) {
    (void) argc, (void) argv;

    hashset set = hashset_create(1019071);

    srand(time(0));

    int max = 100000;

    for (int i = 0; i <= max; i++) {
        int element = rand() % max;
        //printf("Adding: %d\n", element);
        hashset_add(&set, (void*) element);
    }

    int count = 0;

    for (hashset_iter iter = hashset_begin(&set);
         !hashset_is_end(iter);
         iter = hashset_next(iter))
        count++; //printf("Spotting: %d\n", (int) hashset_get(iter));

    printf("Count: %d\n", count);

    hashset_destroy(&set);
}