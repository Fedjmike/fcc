#pragma once

#include "../std/std.h"

typedef enum {
    regUndefined,
    regRAX, /*It is important that every register between RAX and R15 are general registers*/
    regRBX,
    regRCX,
    regRDX,
    regRSI,
    regRDI,
    regR8,
    regR9,
    regR10,
    regR11,
    regR12,
    regR13,
    regR14,
    regR15,
    regRBP,
    regRSP,
    regMax
} regClass;

/**
 * Check if a register is in use
 */
bool regIsUsed (regClass reg);

/**
 * Attempt a lock on a register
 * Returns zero on success, the register itself elsewise
 */
regClass regRequest (regClass reg);

void regFree (regClass reg);

void regFreeAll ();

/**
 * Attempt to allocate a general register, returning its index if successful.
 */
regClass regAllocGeneral ();

/**
 * Return the name of a register as it would be called in assembler source code
 */
char* regToStr (regClass Reg);
