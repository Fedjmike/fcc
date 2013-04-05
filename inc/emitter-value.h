#include "operand.h"

struct ast;
struct emitterCtx;

operand emitterValue (struct emitterCtx* ctx, const struct ast* Node, operand Dest);
