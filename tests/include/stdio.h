using "_stdarg.h";

struct FILE;

FILE* fopen (const char*, const char*);
int fclose (FILE*); 
int fprintf (FILE*, const char*, ...);
int vfprintf (FILE*, const char*, _va_list);
int fputc (int, FILE*);

int printf (const char*, ...);
int puts (const char*);
int putchar (int);

int sprintf (char*, const char*, ...);

extern FILE *stdout, *stdin, *stderr;