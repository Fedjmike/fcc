#pragma once

#include "../std/std.h"

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

    sym* scope;

    ///The levels of break-able control flows currently in
    int breakLevel;

    int errors, warnings;

    ///The last line that an error occurred on
    int lastErrorLine;
} parserCtx;

typedef struct parserResult {
    ast* tree;
    const sym* scope;
    char* filename;
    int errors, warnings;
    bool firsttime, notfound;
} parserResult;

parserResult parser (const char* filename, const char* initialPath, compilerCtx* comp);

void parserResultDestroy (parserResult* result);

ast* parserCode (parserCtx* ctx);
