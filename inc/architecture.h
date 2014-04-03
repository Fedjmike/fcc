#pragma once

#include "../std/std.h"

#include "vector.h"

typedef struct sym sym;

typedef void (*archSymbolMangler)(sym*);

typedef struct architecture {
    int wordsize;
    vector/*<regIndex>*/ scratchRegs, callerSavedRegs;
    archSymbolMangler symbolMangler;
} architecture;

void architectureInit (architecture* arch, int wordsize, archSymbolMangler mangler);
void architectureFree (architecture* arch);
