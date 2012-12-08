#include "type.h"
#include "parser.h"

void errorExpected (parserCtx* ctx, char* Expected);
void errorMismatch (parserCtx* ctx, char* Type, char* One, char* Two);
void errorUndefSym (parserCtx* ctx);
void errorInvalidOp (parserCtx* ctx, char* Op, char* TypeDesc, type DT);
void errorInvalidOpExpected (parserCtx* ctx, char* Op, char* TypeDesc, type DT);

bool tokenIs (parserCtx* ctx, char* Match);

void tokenNext (parserCtx* ctx);

void tokenMatch (parserCtx* ctx);
char* tokenDupMatch (parserCtx* ctx);
void tokenMatchToken (parserCtx* ctx, tokenClass Match);
void tokenMatchStr (parserCtx* ctx, char* Match);
bool tokenTryMatchStr (parserCtx* ctx, char* Match);
int tokenMatchInt (parserCtx* ctx);
char* tokenMatchIdent (parserCtx* ctx);
