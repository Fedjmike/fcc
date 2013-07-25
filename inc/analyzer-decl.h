struct type;
struct ast;

struct analyzerCtx;

void analyzerDecl (struct analyzerCtx* ctx, struct ast* Node);
const struct type* analyzerType (struct analyzerCtx* ctx, struct ast* Node);
