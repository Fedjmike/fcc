#include "../std/std.h"

#include "sym.h"
#include "lexer.h"

struct parserCtx;

sym* scopeSet (struct parserCtx* ctx, sym* Scope);

void errorExpected (struct parserCtx* ctx, const char* Expected);
void errorUndefSym (struct parserCtx* ctx);
void errorUndefType (struct parserCtx* ctx);
void errorIllegalOutside (struct parserCtx* ctx, const char* what, const char* where);
void errorRedeclaredSymAs (struct parserCtx* ctx, sym* Symbol, symTag tag);
void errorReimplementedSym (struct parserCtx* ctx, sym* Symbol);

/*::::*/

bool tokenIsKeyword (const struct parserCtx* ctx, keywordTag keyword);
bool tokenIsPunct (const struct parserCtx* ctx, punctTag punct);
bool tokenIsIdent (const struct parserCtx* ctx);
bool tokenIsInt (const struct parserCtx* ctx);
bool tokenIsString (const struct parserCtx* ctx);
bool tokenIsDecl (const struct parserCtx* ctx);

/*::::*/

void tokenNext (struct parserCtx* ctx);
void tokenSkipMaybe (struct parserCtx* ctx);

/*::::*/

void tokenMatch (struct parserCtx* ctx);
char* tokenDupMatch (struct parserCtx* ctx);

void tokenMatchKeyword (struct parserCtx* ctx, keywordTag keyword);
bool tokenTryMatchKeyword (struct parserCtx* ctx, keywordTag keyword);

void tokenMatchPunct (struct parserCtx* ctx, punctTag punct);
bool tokenTryMatchPunct (struct parserCtx* ctx, punctTag punct);

void tokenMatchToken (struct parserCtx* ctx, tokenTag Match);;

int tokenMatchInt (struct parserCtx* ctx);

char* tokenMatchIdent (struct parserCtx* ctx);
