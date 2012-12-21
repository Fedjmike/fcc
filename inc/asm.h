#pragma once

#include "stdarg.h"
#include "stdio.h"

/**
 * Assembly output context
 *
 * Allocated and initialized by asmInit(), destroyed with asmEnd().
 * Provides file context for all asmXXX functions.
 *
 * @see asmInit() @see asmEnd()
 */
typedef struct {
    ///File being written to
    FILE* file;
    ///Indentation depth level
    int depth;
} asmCtx;

/**
 * Allocate and initialize an asmCtx
 *
 * @param File  The file handle. Ownership not taken, file never
 *              automatically closed
 * @see asmEnd()
 */
asmCtx* asmInit (FILE* File);

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
