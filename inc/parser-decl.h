#include "../std/std.h"

struct ast;

struct parserCtx;

struct ast* parserType (struct parserCtx* ctx);
struct ast* parserDecl (struct parserCtx* ctx, bool module);
