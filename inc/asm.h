#pragma once

#include "operand.h"

#include "stdarg.h"
#include "stdio.h"

struct architecture;

/**
 * Assembly output context
 *
 * Allocated and initialized by asmInit(), destroyed with asmEnd().
 * Provides file context for all asmXXX functions.
 *
 * @see asmInit() @see asmEnd()
 */
typedef struct asmCtx {
    ///File being written to
    FILE* file;
    ///Indentation depth level
    int depth;

    const struct architecture* arch;

    ///(Top of) stack pointer - RSP
    operand stackPtr;
    ///Base (of stack) pointer - RBP
    operand basePtr;
} asmCtx;

/**
 * Allocate and initialize an asmCtx
 * @see asmEnd()
 */
asmCtx* asmInit (const char* output, const struct architecture* arch);

/**
 * Destroy and free an asmCtx
 * @see asmInit()
 */
void asmEnd (asmCtx* ctx);

/**
 * Print a formatted line to the output file. Appends a newline.
 */
void asmOutLn (asmCtx* ctx, char* format, ...);

/**
 * Print a formatted string to the output file. Doesn't append a newline.
 */
void asmVarOut (asmCtx* ctx, char* format, va_list args);

/**
 * Output a formatted comment
 */
void asmComment (asmCtx* ctx, char* format, ...);

/**
 * Enter a block (signalled by indentation)
 * @see asmLeave()
 */
void asmEnter (asmCtx* ctx);

/**
 * Leave a block
 * @see asmEnter()
 */
void asmLeave (asmCtx* ctx);
