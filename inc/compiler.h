#pragma once

#include "vector.h"

typedef struct {
    int errors, warnings;
} compilerResult;

compilerResult compiler (const char* input, const char* output, const vector/*<const char* const>*/ searchPaths);
