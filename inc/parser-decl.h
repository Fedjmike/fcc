#include "../std/std.h"

typedef struct ast ast;

typedef struct parserCtx parserCtx;

ast* parserType (parserCtx* ctx);
ast* parserDecl (parserCtx* ctx, bool module);
ast* parserParam (parserCtx* ctx, bool inDecl);
