#ifndef _H_OCC_STD_STDIO
#define _H_OCC_STD_STDIO

typedef struct
{

	char* _ptr;
	int _cnt;
	char* _base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char* _tmpfname;

} FILE;

FILE* fopen ( char* Path, char* Mode );
int ftell ( FILE* File );
void fseek ( FILE* File, int Offset, int Seek );
int feof ( FILE* File );
void fclose ( FILE* File );

int puts ( char* Str );
int printf ( char* Format, ... );
int sprintf ( char* Str, char* Format, ... );
int fprintf ( FILE* File, char* Format, ... );

char getchar (  );
char fgetc ( FILE* File );

extern int SEEK_SET;
extern int SEEK_CUR;
extern int SEEK_END;

extern FILE* stdout;
extern FILE* stdin;

#endif
