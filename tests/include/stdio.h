struct FILE;

FILE* fopen (char*, char*);

int printf (char*, ...);
int puts (char*);
int putchar (int);

int sprintf (char*, char*, ...);

extern FILE *stdout, *stdin, *stderr;