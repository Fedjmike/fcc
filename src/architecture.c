#include "../inc/architecture.h"

void architectureInit (architecture* arch, int wordsize, archSymbolMangler mangler) {
    arch->wordsize = wordsize;

    vectorInit(&arch->scratchRegs, 4);
    vectorInit(&arch->callerSavedRegs, 4);

    arch->symbolMangler = mangler;
}

void architectureFree (architecture* arch) {
    vectorFree(&arch->scratchRegs);
    vectorFree(&arch->callerSavedRegs);
}
