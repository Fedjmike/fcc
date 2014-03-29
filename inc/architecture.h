#pragma once

#include "../std/std.h"

#include "vector.h"

typedef struct architecture {
    vector/*<regIndex>*/ scratchRegs, callerSavedRegs;
    int wordsize;
} architecture;

void architectureInit (architecture* arch, int wordsize);
void architectureFree (architecture* arch);
