#include "../std/std.h"
#include "../std/stdlib.h"

#include "stdlib.h"
#include "string.h"

//int EXIT_FAILURE = 1;
//int EXIT_SUCCESS = 0;

int SEEK_SET = 0;
int SEEK_CUR = 1;
int SEEK_END = 2;

/* ::::CAST REPLACEMENTS:::: */

int ctoi (char Char) {
    return (int) Char;
}

int* vptoip (void* Ptr) {
    return (int*) Ptr;
}

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

/* ::::MISC:::: s*/

/*Modify a file names extension*/
char* filext (char* Name, char* Extension) {
    /*Location of the first '.' (if any)*/
    int Index = (int)(strchr(Name, (int) ".")-Name);

    if (Index < 0)
        return strcat(strcat(strcpy(malloc(strlen(Name)+5), Name), "."), Extension);

    else
        return strcat(memcpy(malloc(strlen(Name)+4), Name, Index+1), Extension);
}
