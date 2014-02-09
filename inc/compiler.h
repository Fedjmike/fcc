#pragma once

#include "vector.h"

typedef struct compilerResult {
    int errors, warnings;
} compilerResult;

compilerResult compiler (const char* input, const char* output, vector/*<char*>*/* searchPaths);
