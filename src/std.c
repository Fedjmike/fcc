#include "../std/std.h"
#include "../std/stdlib.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

/* ::::MISC:::: */

char* filext (const char* name, const char* extension, void* (*allocator)(size_t)) {
    /*Location of the last '.' (if any)*/
    int index = (int)(strrchr(name, (int) '.') - name);

    char* ret = allocator(strlen(name)+strlen(extension)+2);

    if (extension[0] != 0) {
        if (index < 0)
            sprintf(ret, "%s.%s", name, extension);

        else
            sprintf(ret, "%.*s.%s", index, name, extension);

    } else
        sprintf(ret, "%.*s", index, name);

    return ret;
}

int logi (int x, int base) {
    int n = 0;

    while (x >= base) {
        x /= base;
        n++;
    }

    return n;
}

bool fexists (const char* filename) {
    FILE* file = fopen(filename, "r");

    if (file) {
        fclose(file);
        return true;

    } else
        return false;
}

char* fgetpath (const char* fullname, void* (*allocator)(size_t)) {
    const char* found = strrchr(fullname, (int) '/');

    if (found) {
        int index = (int)(found-fullname);
        char* ret = strncpy(allocator(index+1), fullname, index);
        ret[index] = 0;
        return ret;

    } else
        return strdup(fullname);
}

char* fgetname (const char* fullname, void* (*allocator)(size_t)) {
    const char* found = strrchr(fullname, (int) '/');

    if (found) {
        int index = (int)(found-fullname);
        char* ret = strcpy(allocator(strlen(fullname)-index), fullname+index+1);
        return ret;

    } else
        return strdup(fullname);
}

char* fstripname (const char* fullname, void* (*allocator)(size_t)) {
    char* stripped = allocator(strlen(fullname)+1);
    int copied = 0;

    /*Iterate through the full name, keeping track of:
        - the beginning of this "segment"
        - the slash that ends it
        - the slash that ends the segment after that
      Moving up one segment at a time

      aaaaa/bbbbb/ccccc
      ^    ^     ^
      |    |     |
      |    slash |
      fullname   nextslash*/
    for (const char *slash = fullname-1, *nextslash = strchr(fullname, '/');
         fullname;
         fullname = slash ? slash+1 : 0, slash = nextslash) {
        /*Final segment (no slashes left)*/
        if (!slash) {
            strcpy(stripped+copied, fullname);
            copied += strlen(fullname);

        } else {
            nextslash = strchr(slash+1, '/');

            /*Is the next segment '..'*/
            if (   slash[1] == '.' && slash[2] == '.'
                && (nextslash ? nextslash-slash == 3
                              : slash[3] == 0)) {
                /*Then move past this segment*/
                fullname = slash+1;
                slash = nextslash;

            } else {
                /*Otherwise copy the current segment (with trailing slash)*/
                int segmentLength = (int)(slash-fullname) + 1;
                strncpy(stripped+copied, fullname, segmentLength);
                copied += segmentLength;
            }
        }
    }

    stripped[copied] = 0;
    return stripped;
}

bool strprefix (const char* str, const char* prefix) {
    return !strncmp(str, prefix, strlen(prefix));
}

char* strjoin (char** strs, int n, void* (*allocator)(size_t)) {
    return strjoinwith(strs, n, "", allocator);
}

char* strjoinwith (char** strs, int n, const char* separator, void* (*allocator)(size_t)) {
    if (n <= 0)
        return calloc(1, sizeof(char));

    /*Work out the eventual size*/

    int seplen = strlen(separator),
        firstlen = strlen(strs[0]),
        length = firstlen + seplen*(n-1) + 1;

    for (int i = 1; i < n; i++)
        length += strlen(strs[i]);

    /*Cat them together*/

    char* joined = strcpy(allocator(length), strs[0]);
    int charno = firstlen;

    for (int i = 1; i < n; i++)
        charno += sprintf(joined+charno, "%s%s", separator, strs[i]);

    return joined;
}

int systemf (const char* format, ...) {
    int size = 128;
    char* command = malloc(size);

    va_list args;
    va_start(args, format);
    int length = vsnprintf(command, size, format, args);
    va_end(args);

    if (length < 0 || length >= size) {
        free(command);
        command = malloc(size = length+1);

        va_start(args, format);
        vsnprintf(command, size, format, args);
        va_end(args);
    }

    int ret = system(command);
    free(command);
    return ret;
}

#ifndef max
int max (int x, int y) {
	return x > y ? x : y;
}
#endif

#ifndef min
int min (int x, int y) {
	return x < y ? x : y;
}
#endif
