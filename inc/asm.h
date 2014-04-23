#pragma once

#include "vector.h"
#include "operand.h"

#include "stdio.h"

typedef struct architecture architecture;

typedef enum labelTag {
    labelReturn,
    labelElse,
    labelEndIf,
    labelWhile,
    labelFor,
    labelContinue,
    labelBreak,
    labelShortCircuit,
    labelROData,
    labelLambda,
    labelPostLambda
} labelTag;

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

    const architecture* arch;

    ///(Top of) stack pointer - RSP
    operand stackPtr;
    ///Base (of stack) pointer - RBP
    operand basePtr;

    ///Owns the strings in it
    vector/*<char*>*/ labels;
} asmCtx;

asmCtx* asmInit (const char* output, const architecture* arch);
void asmEnd (asmCtx* ctx);

void asmOutLn (asmCtx* ctx, char* format, ...);

void asmComment (asmCtx* ctx, char* str);

/**
 * Enter a block (signalled by indentation)
 */
void asmEnter (asmCtx* ctx);
void asmLeave (asmCtx* ctx);

operand asmCreateLabel (asmCtx* ctx, labelTag tag);
