#pragma once

#include "compat.h"

char* strdup (const char* Str);

/*Non standard (mine)*/

typedef void* (*stdalloc)(int);

/**
 * Modify a file names extension
 */
char* filext (const char* name, const char* extension);

/**
 * Returns the floor of the log of x, in a given base
 *
 * (int) floor(log((double) x) / log((double) base))
 */
int logi (int x, int base);

bool fexists (const char* filename);

char* fgetpath (const char* fullname);
char* fgetname (const char* fullname);
char* fstripname (const char* fullname);

bool strprefix (const char* str, const char* prefix);

char* strjoin (char** strs, int n, const char* separator, void* (*allocator)(int));

int systemf (const char* format, ...);

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
