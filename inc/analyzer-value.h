#include "../std/std.h"

typedef struct type type;
typedef struct ast ast;

typedef struct analyzerCtx analyzerCtx;

const type* analyzerValue (analyzerCtx* ctx, ast* Node);
void analyzerCompoundInit (analyzerCtx* ctx, ast* Node, const type* DT, bool directInit);
