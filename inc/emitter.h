#include "operand.h"

typedef struct ast ast;
typedef struct architecture architecture;
typedef struct irBlock irBlock;
typedef struct irCtx irCtx;

typedef struct emitterCtx {
    irCtx* ir;
    const architecture* arch;

    irBlock *returnTo, *breakTo, *continueTo;
} emitterCtx;

void emitter (const ast* Tree, const char* output, const architecture* arch);
