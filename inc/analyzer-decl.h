struct type;
struct ast;

struct analyzerCtx;

void analyzerDeclStruct (struct analyzerCtx* ctx, struct ast* Node);
void analyzerDecl (struct analyzerCtx* ctx, struct ast* Node);
const struct type* analyzerType (struct analyzerCtx* ctx, struct ast* Node);
