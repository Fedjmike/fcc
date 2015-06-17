#pragma once

#include "compat.h"

#include "stddef.h"

char* strdup (const char* str);

/*Non standard (mine)*/

typedef void* (*stdalloc)(size_t);

/**
 * Modify a file names extension
 */
char* filext (const char* name, const char* extension, void* (*allocator)(size_t));

/**
 * Returns the floor of the log of x, in a given base
 *
 * (int) floor(log((double) x) / log((double) base))
 */
int logi (int x, int base);

bool fexists (const char* filename);

char* fgetpath (const char* fullname, void* (*allocator)(size_t));
char* fgetname (const char* fullname, void* (*allocator)(size_t));
char* fstripname (const char* fullname, void* (*allocator)(size_t));

bool strprefix (const char* str, const char* prefix);

char* strjoin (char** strs, int n, void* (*allocator)(size_t));
char* strjoinwith (char** strs, int n, const char* separator, void* (*allocator)(size_t));

int systemf (const char* format, ...);

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

#if 0
int max (int x, int y);
int min (int x, int y);
#endif
