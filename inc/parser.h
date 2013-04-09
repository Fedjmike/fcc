#pragma once

#include "lexer.h"

struct ast;
struct sym;

typedef struct parserCtx {
    lexerCtx* lexer;
    tokenLocation location;

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
} parserResult;

parserResult parser (const char* File, struct sym* Global);

struct ast* parserCode (parserCtx* ctx);
