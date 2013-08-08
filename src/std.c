#include "../std/std.h"
#include "../std/stdlib.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

/* ::::STRING.H REPLACEMENTS:::: */

/*int strlen (char* Str)
{

    char* i;

    / *Walk up the string until a null* /
    for (i = Str; *i; i++)
        ;

    return i-Str;

}

char* memcpy (char* Dest, char* Src, int Length)
{


    int i = 0;

    do
        Dest[i] = Src[i];
    while (i++ < Length)

    return Dest;

}

char* strcpy (char* Dest, char* Src)
{

    char* Ret = Dest;

    while (*Src)
        *Dest++ = *Src++;

    *Dest = 0;

    return Ret;

}

/ *Allocate a new copy of a string* /
char* strdup (char* Str)
{

    return strcpy(malloc(strlen(Str)+1), Str);

}

char* strcat (char* Dest, char* Src)
{

    strcpy(Dest+strlen(Dest), Src);

    return Dest;

}

char* strchr (char* Haystack, int Needle)
{

    char* i;

    for (i = Haystack; ctoi(*i) != Needle; i++)
        ;

    return i;

}

int strcmp (char* L, char* R)
{

    while (*L++ == *R++)
        if (*L == 0)
            return 0;

    return *(--L) - *(--R);

}*/

/* ::::CTYPE.H REPLACEMENTS:::: */

/*int isspace (char Look) {
    return Look == ' '  ||
           Look == '\n' ||
           Look == '\r' ||
           Look == '\t';
}

int isalpha (char Look) {
    return (Look >= 'a' && Look <= 'z') || (Look >= 'A' && Look <= 'Z');
}

int isdigit (char Look) {
    return Look >= '0' && Look <= '9';
}

int isalnum (char Look) {
    return isalpha(Look) || isdigit(Look);
}

char tolower (char Char) {
    if (Char >= 'A')
        return Char-('A'-'a');

    else
        return Char;
}*/

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
