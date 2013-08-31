#pragma once

#include "../std/std.h"

#include "vector.h"

struct ast;
struct sym;

struct lexerCtx;

typedef struct {
    char* filename; ///< Only the tokenLocation of the astModule of the file in question owns this
    int line;
    int lineChar;
} tokenLocation;

typedef struct parserCtx {
    struct lexerCtx* lexer;
    tokenLocation location;

    char* filename; ///< Ownership is taken by the tokenLocation of the astModule of the file
    char* fullname;
    vector/*<char*>*/* searchPaths;

    struct sym* scope;

    /*The levels of break-able control flows currently in*/
    int breakLevel;

    int errors;
    int warnings;
} parserCtx;

typedef struct {
    struct ast* tree;
    int errors;
    int warnings;
    bool notfound;
} parserResult;

parserResult parser (const char* filename, struct sym* global, vector/*<char*>*/* searchPaths);

struct ast* parserCode (parserCtx* ctx);
