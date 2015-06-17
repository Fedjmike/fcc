#include "../std/std.h"

using "forward.h";

typedef struct ast ast;

typedef struct parserCtx parserCtx;

ast* parserType (parserCtx* ctx);
ast* parserDecl (parserCtx* ctx, bool module);
void parserParamList (parserCtx* ctx, ast* Node, bool inDecl);
