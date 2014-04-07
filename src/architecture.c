#include "../inc/architecture.h"

#include "../inc/debug.h"
#include "../inc/sym.h"
#include "../inc/reg.h"

#include "../std/std.h"

#include "stdlib.h"
#include "string.h"

static void archSetupRegs (architecture* arch, osTag os);
static void archSetupDriverFlags (architecture* arch, osTag os);

static void manglerLinux (sym* Symbol);
static void manglerWindows (sym* Symbol);

/*:::: CTOR/DTOR ::::*/

void archInit (architecture* arch) {
    arch->wordsize = 0;

    vectorInit(&arch->scratchRegs, 4);
    vectorInit(&arch->calleeSaveRegs, 4);

    arch->symbolMangler = 0;

    arch->asflags = 0;
    arch->ldflags = 0;
}

void archFree (architecture* arch) {
    vectorFree(&arch->scratchRegs);
    vectorFree(&arch->calleeSaveRegs);

    free(arch->asflags);
    free(arch->ldflags);
}

/*:::: SETUP ::::*/

void archSetup (architecture* arch, osTag os, int wordsize) {
    /*Most details got from Agner Fog's calling convention manual;
       - http://www.agner.org/optimize/calling_conventions.pdf */

    arch->wordsize = wordsize;

    /*Calling conventon registers*/

    archSetupRegs(arch, os);

    /*Symbol mangling*/

    if (os == osLinux)
        arch->symbolMangler = manglerLinux;

    else if (os == osWindows)
        arch->symbolMangler = manglerWindows;

    else {
        debugErrorUnhandledInt("archSetup", "OS", (int) os);
        arch->symbolMangler = manglerLinux;
    }

    /*Flags for AS/LD*/

    archSetupDriverFlags(arch, os);
}

static void archSetupRegs (architecture* arch, osTag os) {
    /*32-bit*/
    if (arch->wordsize == 4) {
        vectorPushFromArray(&arch->scratchRegs, (void**) (regIndex[3]) {regRAX, regRCX, regRDX},
                            3, sizeof(regIndex));
        vectorPushFromArray(&arch->calleeSaveRegs, (void**) (regIndex[3]) {regRBX, regRSI, regRDI},
                            3, sizeof(regIndex));

    /*64-bit*/
    } else if (arch->wordsize == 8) {
        /*Pretty similar between OS's, except for RSI and RDI*/

        regIndex scratchRegs[7] = {regRAX, regRCX, regRDX, regR8, regR9, regR10, regR11};
        regIndex calleeSaveRegs[5] = {regRBX, regR12, regR13, regR14, regR15};

        vectorPushFromArray(&arch->scratchRegs, (void**) scratchRegs,
                            sizeof(scratchRegs)/sizeof(regIndex), sizeof(regIndex));
        vectorPushFromArray(&arch->calleeSaveRegs, (void**) calleeSaveRegs,
                            sizeof(scratchRegs)/sizeof(regIndex), sizeof(regIndex));

        /*RSI and RDI are scratch regs on Windows, callee save on most others*/
        vector* RSIandRDI = os == osWindows ? &arch->scratchRegs : &arch->calleeSaveRegs;
        vectorPush(RSIandRDI, (void*) regRSI);
        vectorPush(RSIandRDI, (void*) regRDI);

    } else
        debugErrorUnhandledInt("archSetupRegs", "arch word size", arch->wordsize);
}

static void archSetupDriverFlags (architecture* arch, osTag os) {
    (void) os;

    /*Tell the assembler and linker which arch we are targetting*/

    if (arch->wordsize == 4) {
        arch->asflags = strdup("-m32");
        arch->ldflags = strdup("-m32");

    } else if (arch->wordsize == 8) {
        arch->asflags = strdup("-m64");
        arch->ldflags = strdup("-m64");
    }
}

/*:::: ARCH SPECIFICS ::::*/

static void manglerLinux (sym* Symbol) {
    Symbol->label = strdup(Symbol->ident);
}

static void manglerWindows (sym* Symbol) {
    Symbol->label = malloc(strlen(Symbol->ident)+2);
    sprintf(Symbol->label, "_%s", Symbol->ident);
}
