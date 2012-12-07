#include "stdio.h"

#include "asm.h"

struct ast;

typedef struct {
    asmCtx* Asm;

    operand labelReturnTo;
    operand labelBreakTo;
} emitterCtx;

void emitter (ast* Tree, FILE* File);
