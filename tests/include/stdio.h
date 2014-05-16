struct FILE;

FILE* fopen (const char*, const char*);
int fclose (FILE*);

int fprintf (FILE*, const char*, ...);
int vfprintf (FILE*, const char*, va_list);
int fputs (const char*, FILE*);
int fputc (int, FILE*);

int fseek (FILE*, int, int);
int feof (FILE*);
int fgetc (FILE*);

int printf (const char*, ...);
int vprintf (const char*, va_list);
int puts (const char*);
int putchar (int);

int getchar (void);

int sprintf (char*, const char*, ...);
int snprintf (char*, int, const char*, ...);
int vsnprintf (char*, int, const char*, va_list);

extern FILE *stdout, *stdin, *stderr;

enum {
	SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2
};