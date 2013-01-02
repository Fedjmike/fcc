#ifndef _H_OCC_STD_STD
#define _H_OCC_STD_STD

/*Cast replacements (mine)*/
int ctoi (char Char);
int* vptoip (void* Ptr);

/*/ *string.h* /
int strlen ( char* Str );
char* memcpy ( char* Dest, char* Src, int Length );
char* strcpy ( char* Dest, char* Src );*/
char* strdup ( char* Str );						//Non standard (GNU extension)
/*char* strcat ( char* Dest, char* Src );
char* strchr ( char* Haystack, int Needle );
int strcmp ( char* L, char* R );

/ *ctype.h* /
int isspace ( char Look );
int isalpha ( char Look );
int isdigit ( char Look );
int isalnum ( char Look );
char tolower ( char Char );*/

/*Misc*/
char* filext (char* Name, char* Extension);	//Non standard (mine)

typedef enum {false, true} bool;

#endif
