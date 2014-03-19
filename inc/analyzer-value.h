#include "../std/std.h"

struct type;
struct ast;

struct analyzerCtx;

const struct type* analyzerValue (struct analyzerCtx* ctx, struct ast* Node);
const struct type* analyzerInitOrCompoundLiteral (struct analyzerCtx* ctx, struct ast* Node, const struct type* DT, bool directInit);
