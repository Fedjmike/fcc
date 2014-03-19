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

operand emitterValue (struct emitterCtx* ctx, const struct ast* Node, emitterRequest request);
void emitterInitOrCompoundLiteral (struct emitterCtx* ctx, const struct ast* Node, operand base);
