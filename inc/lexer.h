#include "../std/std.h"

typedef enum {
    tokenUndefined,
    tokenOther,
    tokenEOF,
    tokenIdent,
    tokenInt
} tokenClass;

void lexerInit (char* File);
void lexerEnd ();

void lexerPush (char* File);
void lexerPop ();

void lexerNext ();

void errorExpected (char* Expected);
void errorMismatch (char* Type, char* One, char* Two);
void errorUndefSym ();
void errorInvalidOp (char* Op, char* TypeDesc, type DT);
void errorInvalidOpExpected (char* Op, char* TypeDesc, type DT);

bool lexerIs (char* Match);

void lexerMatch ();
char* lexerDupMatch ();
void lexerMatchToken (tokenClass Match);
void lexerMatchStr (char* Match);
bool lexerTryMatchStr (char* Match);
int lexerMatchInt ();
char* lexerMatchIdent ();

/*Global lexer (tokens) context*/
extern tokenClass lexerToken;
extern char* lexerBuffer;
