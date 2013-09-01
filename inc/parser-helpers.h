#include "../std/std.h"

#include "sym.h"
#include "lexer.h"

struct parserCtx;

sym* scopeSet (struct parserCtx* ctx, sym* Scope);

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

char* tokenMatchStr (struct parserCtx* ctx);
