#include "operand.h"

typedef struct ast ast;
typedef struct emitterCtx emitterCtx;
typedef struct irBlock irBlock;

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
