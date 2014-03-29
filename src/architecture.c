#include "../inc/architecture.h"

void architectureInit (architecture* arch, int wordsize) {
    arch->wordsize = wordsize;

    vectorInit(&arch->scratchRegs, 4);
    vectorInit(&arch->callerSavedRegs, 4);
}

void architectureFree (architecture* arch) {
    vectorFree(&arch->scratchRegs);
    vectorInit(&arch->callerSavedRegs, 4);
}
