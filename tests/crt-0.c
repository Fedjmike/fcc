void* malloc (int);
void free (void*);

int strlen (char*);
char* strcpy (char*, char*);

int puts (char*);

int main () {
	char* str = "hello\n\tworld";
	char* copy = (char*) malloc(strlen(str)+1);
	strcpy(copy, str);
	puts(copy);
	free(copy);
	return 0;
}