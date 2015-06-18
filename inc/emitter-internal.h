#include "operand.h"

typedef struct ast ast;
typedef struct architecture architecture;
typedef struct irBlock irBlock;
typedef struct irFn irFn;
typedef struct irCtx irCtx;
typedef enum regIndex regIndex;

typedef struct emitterCtx {
    irCtx* ir;
    const architecture* arch;

    irFn* curFn;
    irBlock *returnTo, *breakTo, *continueTo;
} emitterCtx;

/*==== emitter-helpers.c ==== Emitter helper functions ====*/

irFn* emitterSetFn (emitterCtx* ctx, irFn* fn);
irBlock* emitterSetReturnTo (emitterCtx* ctx, irBlock* block);
irBlock* emitterSetBreakTo (emitterCtx* ctx, irBlock* block);
irBlock* emitterSetContinueTo (emitterCtx* ctx, irBlock* block);

operand emitterGetInReg (emitterCtx* ctx, irBlock* block, operand src, int size);

/**
 * Forcibly allocate a register, saving its old value on the stack if need be.
 * @see emitterGiveBackReg()
 */
operand emitterTakeReg (emitterCtx* ctx, irBlock* block, regIndex r, int* oldSize, int newSize);
void emitterGiveBackReg (emitterCtx* ctx, irBlock* block, regIndex r, int oldSize);

void emitterBranchOnValue (emitterCtx* ctx, irBlock* block, const ast* value,
                           irBlock* ifTrue, irBlock* ifFalse);

operand emitterWiden (emitterCtx* ctx, irBlock* block, operand R, int size);
operand emitterNarrow (emitterCtx* ctx, irBlock* block, operand R, int size);

void emitterZeroMem (emitterCtx* ctx, irBlock* block, operand L);

int emitterFnAllocateStack (const architecture* arch, sym* fn);

/*==== emitter.c ==== Code generation for blocks and statements ====*/

irBlock* emitterCode (emitterCtx* ctx, irBlock* block, const ast* Node, irBlock* continuation);

/*==== emitter-decl.c ====*/

void emitterDecl (emitterCtx* ctx, irBlock** block, const ast* Node);

/*==== emitter-value.c ==== Code generation for expressions ====*/

typedef enum emitterRequest {
    requestAny,
    ///When you don't care where the value goes
    requestVoid,
    requestReg,
    requestRegOrMem,
    requestMem,
    ///When you need to use the operand as a first-class value in assembly
    requestValue,
    requestArray,
    requestFlags,
    requestReturn,
    requestStack
} emitterRequest;

/**
 * Calculate the value of an astNode, requesting it to go in a certain operand
 * class, returning where it goes.
 */
operand emitterValue (emitterCtx* ctx, irBlock** block, const ast* Node, emitterRequest request);

/**
 * Calculate the value, requesting that it go in a specific, already allocated
 * operand which is then returned.
 */
operand emitterValueSuggest (emitterCtx* ctx, irBlock** block, const ast* Node, const operand* request);

operand emitterSymbol (emitterCtx* ctx, const sym* Symbol);

void emitterCompoundInit (emitterCtx* ctx, irBlock** block, const ast* Node, operand base);
