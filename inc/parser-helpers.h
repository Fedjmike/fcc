#include "../std/std.h"

#include "lexer.h"

struct parserCtx;

void errorExpected (struct parserCtx* ctx, const char* Expected);
void errorUndefSym (struct parserCtx* ctx);
void errorIllegalBreak (struct parserCtx* ctx);
void errorIdentOutsideDecl (struct parserCtx* ctx);
void errorDuplicateSym (struct parserCtx* ctx);

bool tokenIs (struct parserCtx* ctx, const char* Match);
bool tokenIsIdent (struct parserCtx* ctx);
bool tokenIsInt (struct parserCtx* ctx);
bool tokenIsDecl (struct parserCtx* ctx);

void tokenNext (struct parserCtx* ctx);

void tokenMatch (struct parserCtx* ctx);
char* tokenDupMatch (struct parserCtx* ctx);
void tokenMatchToken (struct parserCtx* ctx, tokenClass Match);
void tokenMatchStr (struct parserCtx* ctx, const char* Match);
bool tokenTryMatchStr (struct parserCtx* ctx, const char* Match);
int tokenMatchInt (struct parserCtx* ctx);
char* tokenMatchIdent (struct parserCtx* ctx);
