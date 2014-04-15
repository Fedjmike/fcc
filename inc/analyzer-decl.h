using "forward.h";

typedef struct type type;
typedef struct ast ast;

typedef struct analyzerCtx analyzerCtx;

void analyzerDecl (analyzerCtx* ctx, ast* Node);
const type* analyzerType (analyzerCtx* ctx, ast* Node);
