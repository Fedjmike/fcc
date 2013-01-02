#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/analyzer.h"
#include "../inc/analyzer-value.h"

#include "stdlib.h"
#include "stdarg.h"

static void analyzerModule (analyzerCtx* ctx, ast* Node);

static void analyzerNode (analyzerCtx* ctx, ast* Node);

static void analyzerFunction (analyzerCtx* ctx, ast* Node);
static void analyzerVar (analyzerCtx* ctx, ast* Node);
static void analyzerCode (analyzerCtx* ctx, ast* Node);
static void analyzerBranch (analyzerCtx* ctx, ast* Node);
static void analyzerLoop (analyzerCtx* ctx, ast* Node);
static void analyzerIter (analyzerCtx* ctx, ast* Node);
static void analyzerReturn (analyzerCtx* ctx, ast* Node);

static void analyzerError (analyzerCtx* ctx, const ast* Node, const char* format, ...) {
    printf("error(%d:%d): ", Node->location.line, Node->location.lineChar);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;
}

static void analyzerWarning (analyzerCtx* ctx, const ast* Node, const char* format, ...) {
    printf("warning(%d:%d): ", Node->location.line, Node->location.lineChar);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');

    ctx->warnings++;
}

void analyzerErrorExpected (analyzerCtx* ctx, const ast* Node, const char* where, const char* Expected, const type* Found) {
    char* FoundStr = typeToStr(Found, "");

    analyzerError(ctx, Node, "%s expected %s, found %s",
                  where, Expected, FoundStr);

    free(FoundStr);
}

void analyzerErrorExpectedType (analyzerCtx* ctx, const ast* Node, const char* where, const type* Expected, const type* Found) {
    char* ExpectedStr = typeToStr(Expected, "");

    analyzerErrorExpected(ctx, Node, where, ExpectedStr, Found);

    free(ExpectedStr);
}

void analyzerErrorOp (analyzerCtx* ctx, const ast* Operator, const char* o, const char* desc, const ast* Operand, const type* DT) {
    char* DTStr = typeToStr(DT, "");

    analyzerError(ctx, Operand, "%s requires %s, found %s",
                  o, desc, DTStr);

    free(DTStr);
}

void analyzerErrorMismatch (analyzerCtx* ctx, const ast* Node, const char* o, const type* L, const type* R) {
    char* LStr = typeToStr(L, "");
    char* RStr = typeToStr(R, "");

    analyzerError(ctx, Node, "type mismatch between %s and %s for %s",
                  LStr, RStr, o);

    free(LStr);
    free(RStr);
}

void analyzerErrorDegree (analyzerCtx* ctx, const ast* Node, const char* thing, int expected, int found, const char* where) {
    analyzerError(ctx, Node, "%d %s expected, %d given to %s",
                  expected, thing, found, where);
}

void analyzerErrorParamMismatch (analyzerCtx* ctx, const ast* Node, int n, const type* Expected, const type* Found) {
    char* ExpectedStr = typeToStr(Expected, "");
    char* FoundStr = typeToStr(Found, "");

    analyzerError(ctx, Node, "type mismatch at parameter %d of %s: expected %s, found %s",
                  n, Node->symbol->ident, ExpectedStr, FoundStr);

    free(ExpectedStr);
    free(FoundStr);
}

static analyzerCtx* analyzerInit (sym** Types) {
    analyzerCtx* ctx = malloc(sizeof(analyzerCtx));
    ctx->types = Types;

    ctx->errors = 0;
    ctx->warnings = 0;
    return ctx;
}

static void analyzerEnd (analyzerCtx* ctx) {
    free(ctx);
}

analyzerResult analyzer (ast* Tree, sym* Global, sym** Types) {
    analyzerCtx* ctx = analyzerInit(Types);

    analyzerModule(ctx, Tree);
    analyzerResult result = {ctx->errors, ctx->warnings};

    analyzerEnd(ctx);
    return result;
}

static void analyzerModule (analyzerCtx* ctx, ast* Node) {
    debugEnter("Module");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        analyzerNode(ctx, Current);

    debugLeave();
}

static void analyzerNode (analyzerCtx* ctx, ast* Node) {
    if (Node->class == astEmpty)
        debugMsg("Empty");

    else if (Node->class == astFunction)
        analyzerFunction(ctx, Node);

    else if (Node->class == astVar)
        analyzerVar(ctx, Node);

    else if (Node->class == astCode)
        analyzerCode(ctx, Node);

    else if (Node->class == astBranch)
        analyzerBranch(ctx, Node);

    else if (Node->class == astLoop)
        analyzerLoop(ctx, Node);

    else if (Node->class == astIter)
        analyzerIter(ctx, Node);

    else if (Node->class == astReturn)
        analyzerReturn(ctx, Node);

    else if (astIsValueClass(Node->class))
        /*Throw away return*/
        analyzerValue(ctx, Node);

    else
        debugErrorUnhandled("analyzerNode", "AST class", astClassGetStr(Node->class));
}

static void analyzerFunction (analyzerCtx* ctx, ast* Node) {
    debugEnter("Function");

    ctx->returnType = Node->symbol->dt;

    if (Node->r)
        analyzerNode(ctx, Node->r);

    debugLeave();
}

static void analyzerVar (analyzerCtx* ctx, ast* Node) {
    debugEnter("Var");

    /*Initial value?*/
    if (Node->r) {
        type* R = analyzerValue(ctx, Node->r);

        if (typeIsArray(Node->symbol->dt) && typeIsArray(R) &&
            Node->symbol->dt->array >= R->array &&
            typeIsCompatible(R->base, Node->symbol->dt->base))
            ;

        else if (!typeIsCompatible(R, Node->symbol->dt))
            analyzerErrorExpectedType(ctx, Node->r, "variable initialization", Node->symbol->dt, R);
    }

    debugLeave();
}

static void analyzerCode (analyzerCtx* ctx, ast* Node) {
    debugEnter("Code");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        analyzerNode(ctx, Current);

    debugLeave();
}

static void analyzerBranch (analyzerCtx* ctx, ast* Node) {
    debugEnter("Branch");

    /*Is the condition a valid condition?*/

    ast* cond = Node->firstChild;
    type* condDT = analyzerValue(ctx, cond);

    if (!typeIsCondition(condDT))
        analyzerErrorExpected(ctx, cond, "if", "condition", condDT);

    /*Code*/

    analyzerNode(ctx, Node->l);

    if (Node->r)
        analyzerNode(ctx, Node->r);

    debugLeave();
}

static void analyzerLoop (analyzerCtx* ctx, ast* Node) {
    debugEnter("Loop");

    /*do while?*/
    bool isDo = Node->l->class == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    /*Condition*/

    type* condDT = analyzerValue(ctx, cond);

    if (!typeIsCondition(condDT))
        analyzerErrorExpected(ctx, cond, "do loop", "condition", condDT);

    /*Code*/

    analyzerNode(ctx, code);

    debugLeave();
}

static void analyzerIter (analyzerCtx* ctx, ast* Node) {
    debugEnter("Iter");

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    /*Initializer*/

    if (init->class == astVar)
        analyzerNode(ctx, init);

    else if (init->class != astEmpty)
        analyzerValue(ctx, init);

    /*Condition*/

    type* condDT = analyzerValue(ctx, cond);

    if (!typeIsCondition(condDT))
        analyzerErrorExpected(ctx, cond, "for loop", "condition", condDT);

    /*Iterator*/

    if (iter->class != astEmpty)
        analyzerValue(ctx, iter);

    /*Code*/

    analyzerNode(ctx, Node->l);

    debugLeave();
}

static void analyzerReturn (analyzerCtx* ctx, ast* Node) {
    debugEnter("Return");

    /*Return type, if any, matches?*/

    type* R = typeCreateInvalid();

    if (Node->r)
        R = analyzerValue(ctx, Node->r);

    if (!typeIsCompatible(R, ctx->returnType))
        analyzerErrorExpectedType(ctx, Node->r, "function return", ctx->returnType, R);

    debugLeave();
}

