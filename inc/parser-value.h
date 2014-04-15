using "forward.h";

typedef struct ast ast;

typedef struct parserCtx parserCtx;

ast* parserValue (parserCtx* ctx);
ast* parserAssignValue (parserCtx* ctx);
