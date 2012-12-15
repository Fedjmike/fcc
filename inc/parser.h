#pragma once

#include "lexer.h"

struct ast;
struct sym;

typedef struct {
    int line;
    int lineChar;
} tokenLocation;

typedef struct {
    lexerCtx* lexer;
    tokenLocation location;

    struct sym* scope;

    bool insideLoop;

    int errors;
    int warnings;
} parserCtx;

typedef struct {
    struct ast* tree;
    int errors;
    int warnings;
} parserResult;

parserResult parser (char* File, struct sym* Global);
