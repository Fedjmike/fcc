#pragma once

#include "../std/std.h"

#include "hashmap.h"

using "forward.h";

using "hashmap.h";

typedef struct vector vector;
typedef struct architecture architecture;
typedef struct sym sym;

/**
 * Indices of certain built in symbols for compilerCtx::types.
 * Used by the analyzer.
 */
enum {
    builtinVoid,
    builtinBool,
    builtinChar,
    builtinInt,
    builtinSizeT,
    builtinVAList,
    builtinTotal
};

typedef struct compilerCtx {
    sym* global;
    sym** types;

    hashmap/*<parserResult*>*/ modules;

    const architecture* arch;
    const vector/*<char*>*/* searchPaths;

    int errors, warnings;
} compilerCtx;

void compilerInit (compilerCtx* ctx, const architecture* arch, const vector/*<char*>*/* searchPaths);
void compilerEnd (compilerCtx* ctx);

void compiler (compilerCtx* ctx, const char* input, const char* output);
