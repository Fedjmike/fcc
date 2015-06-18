#pragma once

#include "../std/std.h"

#include "lexer.h"

typedef struct vector vector;
typedef struct ast ast;
typedef struct sym sym;
typedef struct compilerCtx compilerCtx;
typedef struct lexerCtx lexerCtx;

typedef struct tokenLocation {
    const char* filename;
    int line;
    int lineChar;
} tokenLocation;

typedef struct parserCtx {
    lexerCtx* lexer;
    tokenLocation location;

    ///Ownership of filename is taken by the parserResult of the astModule of the file
    char* filename;
    const char* fullname;
    char* path;

    compilerCtx* comp;

    ///Links to the module level and innermost scope
    ///Owned by the symbol tree
    sym* module;
    sym* scope;

    ///The levels of break-able control flows currently in
    int breakLevel;

    int errors, warnings;

    ///The last line that an error occurred on
    int lastErrorLine;
} parserCtx;

/*==== parser-helpers.c ====*/

sym* scopeSet (parserCtx* ctx, sym* scope);

bool tokenIsKeyword (const parserCtx* ctx, keywordTag keyword);
bool tokenIsPunct (const parserCtx* ctx, punctTag punct);
bool tokenIsIdent (const parserCtx* ctx);
bool tokenIsInt (const parserCtx* ctx);
bool tokenIsString (const parserCtx* ctx);
bool tokenIsChar (const parserCtx* ctx);
bool tokenIsDecl (const parserCtx* ctx);

void tokenNext (parserCtx* ctx);
void tokenSkipMaybe (parserCtx* ctx);

void tokenMatch (parserCtx* ctx);
char* tokenDupMatch (parserCtx* ctx);

void tokenMatchToken (parserCtx* ctx, tokenTag Match);
void tokenMatchKeyword (parserCtx* ctx, keywordTag keyword);
void tokenMatchPunct (parserCtx* ctx, punctTag punct);
bool tokenTryMatchKeyword (parserCtx* ctx, keywordTag keyword);
bool tokenTryMatchPunct (parserCtx* ctx, punctTag punct);

int tokenMatchInt (parserCtx* ctx);
char* tokenMatchIdent (parserCtx* ctx);
char* tokenMatchStr (parserCtx* ctx);
char tokenMatchChar (parserCtx* ctx);

/*==== parser.c ==== High level parsing, statements and blocks ====*/

ast* parserCode (parserCtx* ctx);

/*==== parser-value.c ==== Expression parsing ====*/

ast* parserValue (parserCtx* ctx);
ast* parserAssignValue (parserCtx* ctx);

/*==== parser-decl.c ==== Declarations and type specifications ====*/

ast* parserType (parserCtx* ctx);
ast* parserDecl (parserCtx* ctx, bool module);
void parserParamList (parserCtx* ctx, ast* Node, bool inDecl);
