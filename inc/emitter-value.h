#include "operand.h"

struct ast;
struct emitterCtx;

operand emitterValue (emitterCtx* ctx, ast* Node, operand Dest);
