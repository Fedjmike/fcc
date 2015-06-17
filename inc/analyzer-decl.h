#include "../std/std.h"

using "forward.h";

typedef struct type type;
typedef struct ast ast;

typedef struct analyzerCtx analyzerCtx;

void analyzerDecl (analyzerCtx* ctx, ast* Node, bool module);
const type* analyzerType (analyzerCtx* ctx, ast* Node);
type* analyzerParamList (analyzerCtx* ctx, ast* Node, type* returnType);
