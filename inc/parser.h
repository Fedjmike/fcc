#pragma once

#include "../std/std.h"

typedef struct vector vector;
typedef struct ast ast;
typedef struct sym sym;
typedef struct lexerCtx lexerCtx;

typedef struct tokenLocation {
    char* filename; ///< Only the tokenLocation of the astModule of the file in question owns this
    int line;
    int lineChar;
} tokenLocation;

typedef struct parserCtx {
    lexerCtx* lexer;
    tokenLocation location;

    char* filename; ///< Ownership is taken by the tokenLocation of the astModule of the file
    char* fullname;
    char* path;
    vector/*<char*>*/* searchPaths;

    sym* scope;

    /*The levels of break-able control flows currently in*/
    int breakLevel;

    int errors;
    int warnings;
} parserCtx;

typedef struct parserResult {
    struct ast* tree;
    int errors;
    int warnings;
    bool notfound;
} parserResult;

parserResult parser (const char* filename, struct sym* global,
                     const char* initialPath, vector/*<char*>*/* searchPaths);

ast* parserCode (parserCtx* ctx);
