#pragma once

#include "../std/std.h"

#include "vector.h"

struct ast;
struct sym;

struct lexerCtx;

typedef struct {
    const char* filename;
    int line;
    int lineChar;
} tokenLocation;

typedef struct parserCtx {
    struct lexerCtx* lexer;
    tokenLocation location;

    char* filename; ///< Ownership is taken by the parserResult of the astModule of the file
    const char* fullname;
    char* path;

    compilerCtx* comp;

    struct sym* scope;

    /*The levels of break-able control flows currently in*/
    int breakLevel;

    int errors;
    int warnings;

    /*The last line that an error occurred on*/
    int lastErrorLine;
} parserCtx;

typedef struct parserResult {
    struct ast* tree;
    char* filename;
    int errors, warnings;
    bool firsttime, notfound;
} parserResult;

parserResult parser (const char* filename, const char* initialPath, compilerCtx* comp);

struct ast* parserCode (parserCtx* ctx);
