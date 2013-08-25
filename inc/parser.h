#pragma once

#include "../std/std.h"

#include "vector.h"
#include "lexer.h"

struct ast;
struct sym;

typedef struct parserCtx {
    lexerCtx* lexer;
    tokenLocation location;

    const vector/*<const char* const>*/ searchPaths;

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

parserResult parser (const char* filename, struct sym* global, const vector/*<const char* const>*/ searchPaths);

struct ast* parserCode (parserCtx* ctx);
