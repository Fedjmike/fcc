#include "operand.h"

typedef struct ast ast;
typedef struct architecture architecture;
typedef struct asmCtx asmCtx;

typedef struct emitterCtx {
    asmCtx* Asm;
    const architecture* arch;

    operand labelReturnTo;
    operand labelBreakTo;
    operand labelContinueTo;
} emitterCtx;

void emitter (const ast* Tree, const char* output, const architecture* arch);

int emitterFnAllocateStack (const architecture* arch, sym* fn);
void emitterCode (emitterCtx* ctx, const ast* Node);
