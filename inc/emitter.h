#include "operand.h"

typedef struct ast ast;
typedef struct architecture architecture;
typedef struct irBlock irBlock;
typedef struct irFn irFn;
typedef struct irCtx irCtx;

typedef struct emitterCtx {
    irCtx* ir;
    const architecture* arch;

    irFn* curFn;
    irBlock *returnTo, *breakTo, *continueTo;
} emitterCtx;

void emitter (const ast* Tree, const char* output, const architecture* arch);

int emitterFnAllocateStack (const architecture* arch, sym* fn);
irBlock* emitterCode (emitterCtx* ctx, irBlock* block, const ast* Node, irBlock* continuation);
