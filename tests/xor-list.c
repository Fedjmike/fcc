#include "compat.h"
#include "stdlib.h"
#include "stdio.h"

void* malloc (int);
void free (void*);

int printf (char*, ...);
int puts (char*);

//using(stdlib);
//using(stdio);

typedef struct list_node {
    int item;
    //Simply next or previous for first and last nodes
    int xor_ptr;
} list_node;

list_node* list_init (int array[], int length);
list_node* node_init (int item, int xor_ptr);
list_node* get_last (list_node* first);
bool node_is_last (list_node* previous, list_node* current);
void advance (list_node** previous, list_node** current);
list_node* insert_between (list_node* previous, list_node* next, int item);

/**
 * Create a new list out of a non-empty array
 *
 * If an empty array is given, returns a null ptr
 */
list_node* list_init (int array[], int length) {
    if (length == 0)
        return (list_node*) 0;

    else {
        //Inserting between two null pointers works (yay)
        list_node* current = (list_node*) 0;
        
        //Do it backwards, add the item to the beginning
        //Because we want to end up with & return the first!
        for (int i = length-1; i >= 0; i--)
            current = insert_between((list_node*) 0, current, array[i]);
            
        return current;
    }
}

/**
 * Create a new node with specified fields
 */
list_node* node_init (int item, int xor_ptr) {
    list_node* node = malloc(sizeof(list_node));
    node->item = item;
    node->xor_ptr = xor_ptr;
    return node;
}

/**
 * Return the last element in a list, given the first
 */
list_node* get_last (list_node* first) {
    if (first->xor_ptr == 0)
        return first;

    else {
        list_node *current = first, *previous = 0;

        while (current != 0)
            advance(&previous, &current);

        return previous;
    }
}

/**
 * Checks whether a node is the last element based on it and the previous
 */
bool node_is_last (list_node* previous, list_node* current) {
    return (list_node*) current->xor_ptr == previous;
}

/**
 * Advance along a list (in either direction)
 *
 * Updates two given fields for the previous and current node
 *
 * Advances in the direction implied by the relative positions of
 * previous and current.
 *
 * If current == 0 (i.e. has just moved past the last node),
 * both are set to zero.
 *
 * CAREFUL! Don't mutate the list nodes other than their payload
 * as you go through them.
 */
void advance (list_node** previous, list_node** current) {
    if (*current == 0)
        *previous = 0;

    else {
        list_node* next = (list_node*) ((*current)->xor_ptr ^ (int) *previous);

        *previous = *current;
        *current = next;
    }
}

/**
 * Insert an element between two nodes
 *
 * If either previous or last is null, beginning/end of list assumed
 * Returns the new node
 */
list_node* insert_between (list_node* previous, list_node* next, int item) {
    //Insert into empty list
    if (previous == 0 && next == 0)
        return node_init(item, 0);

    //Insert before first
    else if (previous == 0) {
        list_node* node = node_init(item, (int) next);
        //The old first's xor_ptr is simply the old second
        //So simply xor in the new first
        next->xor_ptr ^= (int) node;
        return node;

    //Insert after last
    } else if (next == 0) {
        list_node* node = node_init(item, (int) previous);
        previous->xor_ptr ^= (int) node;
        return node;

    //Insert between two elements
    } else {
        list_node* node = node_init(item, (int) previous ^ (int) next);
        //xor out one pointer, xor in another
        previous->xor_ptr ^= (int) next ^ (int) node;
        next->xor_ptr ^= (int) previous ^ (int) node;
        return node;
    }
}

/**
 * Delete a node from a list, given its neighbour
 *
 * Middle is the node to be deleted, previous is either neighbour
 */
void node_delete (list_node* previous, list_node* middle) {
    list_node* next = (list_node*) (middle->xor_ptr ^ (int) previous);
    //Swap out the list_node to be deleted, the new couple in
    previous->xor_ptr ^= (int) middle ^ (int) next;
    next->xor_ptr ^= (int) middle ^ (int) previous;

    free(middle);
}

int main () {
    int array[6] = {0, 1, 2, 3, 4, 5};

    list_node* first = list_init(array, 6);
    list_node *fourth, *fifth; //We'll iterate through the list and get these
    
    //Iterate through the list
    for (list_node *current = first, *previous = 0;
         current != 0;
         advance(&previous, &current)) {
        //Print off the current item
        printf("%d  ", current->item);

        if (current->item == 4) {
            fourth = previous;
            fifth = current;
        }
    }

    puts("\nInserting 6, deleting 3");
    list_node* newnode = insert_between(fourth, fifth, 6);
    node_delete(newnode, fourth);

    //Print them off again, backwards!
    for (list_node *current = get_last(first), *previous = 0;
         current != 0;
         advance(&previous, &current))
        printf("%d  ", current->item);

    puts("");
        
    return 0;
}
