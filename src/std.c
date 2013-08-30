#include "../std/std.h"
#include "../std/stdlib.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

/* ::::MISC:::: */

char* filext (const char* name, const char* extension) {
    /*Location of the last '.' (if any)*/
    int index = (int)(strrchr(name, (int) '.') - name);

    char* ret = malloc(strlen(name)+strlen(extension)+2);

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
    int n;

    for (n = 0; x >= base; n++)
        x /= base;

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

char* fgetpath (const char* fullname) {
    int index = (int)(strrchr(fullname, (int) '/') - fullname);
    char* ret = strncpy(malloc(index+1), fullname, index);
    ret[index] = 0;
    return ret;
}

char* fgetname (const char* fullname) {
    int index = (int)(strrchr(fullname, (int) '/') - fullname);
    char* ret = strcpy(malloc(strlen(fullname)-index), fullname+index+1);
    return ret;
}

bool strprefix (const char* str, const char* prefix) {
    return !strncmp(str, prefix, strlen(prefix));
}

int vsystem (const char* format, ...) {
    int size = 128;
    char* command = malloc(size);

    va_list args;
    va_start(args, format);

    int length = vsnprintf(command, size, format, args);

    if (length < 0 || length >= size) {
        free(command);
        command = malloc(size = length+1);
        vsnprintf(command, size, format, args);
    }

    va_end(args);

    int ret = system(command);
    free(command);
    return ret;
}
