#include "operand.h"

struct ast;
struct sym;
struct emitterCtx;

operand emitterValue (struct emitterCtx* ctx, const struct ast* Node, operand Dest);
void emitterInitOrCompoundLiteral (struct emitterCtx* ctx, const struct ast* Node, struct sym* Symbol, operand base);
