#include "../std/std.h"

#include "lexer.h"

struct sym;

struct parserCtx;

struct sym* scopeSet (struct parserCtx* ctx, struct sym* Scope);

void errorExpected (struct parserCtx* ctx, const char* Expected);
void errorUndefSym (struct parserCtx* ctx);
void errorIllegalOutside (struct parserCtx* ctx, const char* what, const char* where);
void errorDuplicateSym (struct parserCtx* ctx);
void errorRedefinedSym (struct parserCtx* ctx, struct sym* Symbol);

bool tokenIs (const struct parserCtx* ctx, const char* Match);
bool tokenIsIdent (const struct parserCtx* ctx);
bool tokenIsInt (const struct parserCtx* ctx);
bool tokenIsString (const struct parserCtx* ctx);
bool tokenIsDecl (const struct parserCtx* ctx);

void tokenNext (struct parserCtx* ctx);

void tokenMatch (struct parserCtx* ctx);
char* tokenDupMatch (struct parserCtx* ctx);
void tokenMatchToken (struct parserCtx* ctx, tokenTag Match);
void tokenMatchStr (struct parserCtx* ctx, const char* Match);
bool tokenTryMatchStr (struct parserCtx* ctx, const char* Match);
int tokenMatchInt (struct parserCtx* ctx);
char* tokenMatchIdent (struct parserCtx* ctx);
