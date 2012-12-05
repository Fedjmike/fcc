void streamInit (char* File);
void streamEnd ();

void streamPush (char* File);
void streamPop ();

/*Load the next character, return the old (!) character*/
char streamNext ();

/*Load backtrack a single character, return the old character (that is now next)*/
char streamPrev ();

int streamGetPos ();

extern char* streamFilename;
extern char streamChar;
