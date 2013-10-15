#pragma once

#include "vector.h"
#include "hashmap.h"

typedef struct {
    architecture arch;

    const vector/*<char*>*/ searchPaths;

    hashmap/*<char*, sym*>*/ modules;
} compilerCtx;

typedef struct {
    int errors, warnings;
} compilerResult;

compilerCtx* compilerInit ();
void compilerEnd (compilerCtx* ctx);

compilerResult compiler (compilerCtx* ctx, const char* input, const char* output, const char* parent);
