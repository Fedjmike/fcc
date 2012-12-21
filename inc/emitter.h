#include "operand.h"
#include "asm.h"

#include "stdio.h"

struct ast;

typedef struct {
    asmCtx* Asm;

    operand labelReturnTo;
    operand labelBreakTo;
} emitterCtx;

void emitter (struct ast* Tree, FILE* File);
