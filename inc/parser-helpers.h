#include "type.h"
#include "lexer.h"

struct parserCtx;

void errorExpected (parserCtx* ctx, const char* Expected);
void errorMismatch (parserCtx* ctx, const char* Type, const char* L, const char* R);
void errorUndefSym (parserCtx* ctx);
void errorInvalidOp (parserCtx* ctx, char* Op, char* TypeDesc, type DT);
void errorInvalidOpExpected (parserCtx* ctx, char* Op, char* TypeDesc, type DT);

bool tokenIs (parserCtx* ctx, const char* Match);

void tokenNext (parserCtx* ctx);

void tokenMatch (parserCtx* ctx);
char* tokenDupMatch (parserCtx* ctx);
void tokenMatchToken (parserCtx* ctx, tokenClass Match);
void tokenMatchStr (parserCtx* ctx, const char* Match);
bool tokenTryMatchStr (parserCtx* ctx, const char* Match);
int tokenMatchInt (parserCtx* ctx);
char* tokenMatchIdent (parserCtx* ctx);
