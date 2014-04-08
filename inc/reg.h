#pragma once

#include "../std/std.h"

typedef struct reg {
    ///Minimum size in bytes
    int size;
    ///Name when a byte, word, dword and qword
    const char* names[4];
    ///If unused, 0, else the size allocated as in bytes
    int allocatedAs;
} reg;

typedef enum regIndex {
    regUndefined,
    /*It is important that every register between RAX and R15 is a general register*/
    regRAX,
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

const char* regIndexGetName (regIndex r, int size);

/**
 * Return the name of a register at a certain size in bytes as it would
 * be called in assembler source code
 */
const char* regGetStr (const reg* r);
