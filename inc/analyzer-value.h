#include "../std/std.h"

struct type;
struct ast;

struct analyzerCtx;

typedef struct value {
    const struct type* dt;
    bool lvalue;
} valueResult;

valueResult analyzerValue (struct analyzerCtx* ctx, struct ast* Node);
