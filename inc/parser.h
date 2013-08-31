#pragma once

#include "../std/std.h"

#include "vector.h"
#include "lexer.h"

struct ast;
struct sym;

typedef struct parserCtx {
    lexerCtx* lexer;
    tokenLocation location;

    const char* filename;
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
