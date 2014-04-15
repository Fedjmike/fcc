#include "../std/std.h"

#include "sym.h"
#include "lexer.h"

using "forward.h";

using "sym.h";
using "lexer.h";

typedef struct parserCtx parserCtx;

sym* scopeSet (parserCtx* ctx, sym* scope);

/*::::*/

bool tokenIsKeyword (const parserCtx* ctx, keywordTag keyword);
bool tokenIsPunct (const parserCtx* ctx, punctTag punct);
bool tokenIsIdent (const parserCtx* ctx);
bool tokenIsInt (const parserCtx* ctx);
bool tokenIsString (const parserCtx* ctx);
bool tokenIsChar (const parserCtx* ctx);
bool tokenIsDecl (const parserCtx* ctx);

/*::::*/

void tokenNext (parserCtx* ctx);
void tokenSkipMaybe (parserCtx* ctx);

/*::::*/

void tokenMatch (parserCtx* ctx);
char* tokenDupMatch (parserCtx* ctx);

void tokenMatchKeyword (parserCtx* ctx, keywordTag keyword);
bool tokenTryMatchKeyword (parserCtx* ctx, keywordTag keyword);

void tokenMatchPunct (parserCtx* ctx, punctTag punct);
bool tokenTryMatchPunct (parserCtx* ctx, punctTag punct);

void tokenMatchToken (parserCtx* ctx, tokenTag Match);

int tokenMatchInt (parserCtx* ctx);

char* tokenMatchIdent (parserCtx* ctx);

char* tokenMatchStr (parserCtx* ctx);

char tokenMatchChar (parserCtx* ctx);
