#include "operand.h"
#include "asm.h"

struct ast;

typedef struct emitterCtx {
    asmCtx* Asm;
    const struct architecture* arch;

    operand labelReturnTo;
    operand labelBreakTo;
    operand labelContinueTo;
} emitterCtx;

void emitter (const struct ast* Tree, const char* output, const struct architecture* arch);
