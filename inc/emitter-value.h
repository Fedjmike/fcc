#include "operand.h"

struct ast;
struct emitterCtx;

operand emitterValue (struct emitterCtx* ctx, struct ast* Node, operand Dest);
