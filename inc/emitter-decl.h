#include "../std/std.h"

using "forward.h";

typedef struct ast ast;
typedef struct irBlock irBlock;
typedef struct emitterCtx emitterCtx;

void emitterDecl (emitterCtx* ctx, irBlock** block, const ast* Node);
