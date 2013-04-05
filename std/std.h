#ifndef _H_OCC_STD_STD
#define _H_OCC_STD_STD

/*/ *string.h* /
int strlen (char* Str);
char* memcpy (char* Dest, char* Src, int Length);
char* strcpy (char* Dest, char* Src);*/
char* strdup (const char* Str);						//Non standard (GNU extension)
/*char* strcat (char* Dest, char* Src);
char* strchr (char* Haystack, int Needle);
int strcmp (char* L, char* R);

/ *ctype.h* /
int isspace (char Look);
int isalpha (char Look);
int isdigit (char Look);
int isalnum (char Look);
char tolower (char Char);*/

/*Non standard (mine)*/

/**
 * Modify a file names extension
 */
char* filext (const char* name, const char* extension);

/**
 * Returns (int) floor(log((double) x) / log((double) base))
 */
int logi (int x, int base);

typedef enum {false = 0, true = 1} bool;

#endif
