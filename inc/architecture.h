#pragma once

#include "../std/std.h"

#include "vector.h"

typedef struct sym sym;

typedef void (*archSymbolMangler)(sym*);

typedef enum osTag {
    osLinux,
    osWindows
} osTag;

typedef struct architecture {
    int wordsize;
    vector/*<regIndex>*/ scratchRegs, calleeSaveRegs;
    archSymbolMangler symbolMangler;
} architecture;

void archInit (architecture* arch);
void archFree (architecture* arch);

void archSetup (architecture* arch, osTag os, int wordsize);
