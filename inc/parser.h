#pragma once

#include "../std/std.h"

typedef struct ast ast;
typedef struct sym sym;
typedef struct compilerCtx compilerCtx;

typedef struct parserResult {
    ast* tree;
    const sym* scope;
    char* filename;
    int errors, warnings;
    bool firsttime, notfound;
} parserResult;

parserResult parser (const char* filename, const char* initialPath, compilerCtx* comp);

void parserResultDestroy (parserResult* result);
