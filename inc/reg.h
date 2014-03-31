#pragma once

#include "../std/std.h"

typedef struct reg {
    int size; /*Minimum size in bytes*/
    const char* names[4]; /*Name when a byte, word, dword and qword*/
    int allocatedAs; /*If unused, 0, else the size allocated as in bytes*/
} reg;

typedef enum regIndex {
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
} regIndex;

extern reg regs[];

/**
 * Check if a register is in use
 */
bool regIsUsed (regIndex r);

const reg* regGet (regIndex r);

/**
 * Attempt a lock on a register
 * Returns the register on success, 0 elsewise
 */
reg* regRequest (regIndex r, int size);

void regFree (reg* r);

/**
 * Attempt to allocate a register, returning it if successful.
 */
reg* regAlloc (int size);

const char* regGetName (regIndex r, int size);

/**
 * Return the name of a register at a certain size in bytes as it would
 * be called in assembler source code
 */
const char* regToStr (const reg* r);
