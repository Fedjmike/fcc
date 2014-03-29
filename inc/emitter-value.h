#include "operand.h"

struct ast;
struct emitterCtx;

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
operand emitterValue (struct emitterCtx* ctx, const struct ast* Node, emitterRequest request);

/**
 * Calculate the value, requesting that it go in a specific, already allocated
 * operand which is then returned.
 */
operand emitterValueSuggest (struct emitterCtx* ctx, const struct ast* Node, const operand* request);

void emitterInitOrCompoundLiteral (struct emitterCtx* ctx, const struct ast* Node, operand base);
