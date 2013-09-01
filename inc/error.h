#include "sym.h"

struct type;
struct ast;

struct parserCtx;
struct analyzerCtx;

void errorExpected (struct parserCtx* ctx, const char* expected);
void errorUndefSym (struct parserCtx* ctx);
void errorUndefType (struct parserCtx* ctx);
void errorIllegalOutside (struct parserCtx* ctx, const char* what, const char* where);
void errorRedeclaredSymAs (struct parserCtx* ctx, sym* Symbol, symTag tag);
void errorReimplementedSym (struct parserCtx* ctx, sym* Symbol);
void errorFileNotFound (struct parserCtx* ctx, const char* name);

void errorTypeExpected (struct analyzerCtx* ctx, const struct ast* Node, const char* where, const char* expected, const struct type* found);
void errorTypeExpectedType (struct analyzerCtx* ctx, const struct ast* Node, const char* where, const struct type* expected, const struct type* found);
void errorOp (struct analyzerCtx* ctx, const char* o, const char* desc, const struct ast* Operand, const struct type* DT);
void errorLvalue (struct analyzerCtx* ctx, const char* o, const struct ast* Operand);
void errorMismatch (struct analyzerCtx* ctx, const struct ast* Node, const char* o, const struct type* L, const struct type* R);
void errorDegree (struct analyzerCtx* ctx, const struct ast* Node, const char* thing, int expected, int found, const char* where);
void errorMember (struct analyzerCtx* ctx, const char* o, const struct ast* Node, const struct type* Record);
void errorParamMismatch (struct analyzerCtx* ctx, const struct ast* Node, int n, const struct type* expected, const struct type* found);
void errorConflictingDeclarations (struct analyzerCtx* ctx, const struct ast* Node, const struct sym* Symbol, const struct type* found);
void errorRedeclaredVar (struct analyzerCtx* ctx, const struct ast* Node, const struct sym* Symbol);
void errorIllegalSymAsValue (struct analyzerCtx* ctx, const struct ast* Node, const struct sym* Symbol);
