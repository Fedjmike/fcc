#include "operand.h"

typedef struct ast ast;
typedef struct emitterCtx emitterCtx;

typedef enum emitterRequest {
    requestAny,
    requestReg,
    requestMem,
    requestOperable,
    requestFlags,
    requestStack
} emitterRequest;

/**
 * Calculate the value of an astNode, requesting it to go in a certain operand
 * class, returning where it goes.
 */
operand emitterValue (emitterCtx* ctx, const ast* Node, emitterRequest request);

/**
 * Calculate the value, requesting that it go in a specific, already allocated
 * operand which is then returned.
 */
operand emitterValueSuggest (emitterCtx* ctx, const ast* Node, const operand* request);

void emitterCompoundInit (emitterCtx* ctx, const ast* Node, operand base);
