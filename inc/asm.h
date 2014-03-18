#pragma once

#include "operand.h"

#include "stdarg.h"
#include "stdio.h"

struct architecture;

/**
 * Assembly output context
 */
typedef struct asmCtx {
    ///File being written to
    char* filename;
    FILE* file;
    ///Indentation depth level
    int depth;

    int lineNo;

    const struct architecture* arch;

    ///(Top of) stack pointer - RSP
    operand stackPtr;
    ///Base (of stack) pointer - RBP
    operand basePtr;
} asmCtx;


asmCtx* asmInit (const char* output, const struct architecture* arch);
void asmEnd (asmCtx* ctx);

void asmOutLn (asmCtx* ctx, char* format, ...);

void asmComment (asmCtx* ctx, char* str);

/**
 * Enter a block (signalled by indentation)
 */
void asmEnter (asmCtx* ctx);
void asmLeave (asmCtx* ctx);
