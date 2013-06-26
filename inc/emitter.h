#include "architecture.h"
#include "operand.h"
#include "asm.h"

#include "stdio.h"

struct ast;

typedef struct emitterCtx {
    asmCtx* Asm;
    const architecture* arch;

    operand labelReturnTo;
    operand labelBreakTo;
} emitterCtx;

void emitter (const struct ast* Tree, FILE* File, const architecture* arch);
