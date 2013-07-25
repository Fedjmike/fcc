struct ast;

struct parserCtx;

struct ast* parserType (struct parserCtx* ctx);
struct ast* parserModuleDecl (struct parserCtx* ctx);
struct ast* parserDecl (struct parserCtx* ctx);
