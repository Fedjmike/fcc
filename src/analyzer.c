#include "../inc/analyzer.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"
#include "../inc/architecture.h"

#include "../inc/compiler.h"

#include "../inc/analyzer-value.h"
#include "../inc/analyzer-decl.h"

#include "stdlib.h"
#include "stdarg.h"

static void analyzerModule (analyzerCtx* ctx, ast* Node);

static void analyzerUsing (analyzerCtx* ctx, ast* Node);
static void analyzerFnImpl (analyzerCtx* ctx, ast* Node);
static void analyzerCode (analyzerCtx* ctx, ast* Node);
static void analyzerBranch (analyzerCtx* ctx, ast* Node);
static void analyzerLoop (analyzerCtx* ctx, ast* Node);
static void analyzerIter (analyzerCtx* ctx, ast* Node);
static void analyzerReturn (analyzerCtx* ctx, ast* Node);

static analyzerCtx* analyzerInit (sym** Types, const architecture* arch) {
    analyzerCtx* ctx = malloc(sizeof(analyzerCtx));
    ctx->types = Types;
    ctx->arch = arch;

    ctx->fnctx.fn = 0;
    ctx->fnctx.returnType = 0;

    intsetInit(&ctx->incompleteDeclIgnore, 16);
    intsetInit(&ctx->incompletePtrIgnore, 16);

    ctx->errors = 0;
    ctx->warnings = 0;
    return ctx;
}

static void analyzerEnd (analyzerCtx* ctx) {
    intsetFree(&ctx->incompleteDeclIgnore);
    intsetFree(&ctx->incompletePtrIgnore);
    free(ctx);
}

static analyzerFnCtx analyzerPushFnctx (analyzerCtx* ctx, sym* Symbol) {
    type* ret;
    const type* fn = typeGetCallable(Symbol->dt);

    if (fn)
        ret = typeDeriveReturn(fn);

    else
        ret = typeCreateInvalid();

    analyzerFnCtx old = ctx->fnctx;
    ctx->fnctx = (analyzerFnCtx) {Symbol, ret};
    return old;
}

static void analyzerPopFnctx (analyzerCtx* ctx, analyzerFnCtx old) {
    typeDestroy(ctx->fnctx.returnType);
    ctx->fnctx = old;
}

analyzerResult analyzer (ast* Tree, sym** Types, const architecture* arch) {
    analyzerCtx* ctx = analyzerInit(Types, arch);

    analyzerNode(ctx, Tree);
    analyzerResult result = {ctx->errors, ctx->warnings};

    analyzerEnd(ctx);
    return result;
}

void analyzerNode (analyzerCtx* ctx, ast* Node) {
    debugEnter(astTagGetStr(Node->tag));

    if (Node->tag == astEmpty)
        debugMsg("Empty");

    else if (Node->tag == astInvalid)
        debugMsg("Invalid");

    else if (Node->tag == astModule)
        analyzerModule(ctx, Node);

    else if (Node->tag == astUsing)
        analyzerUsing(ctx, Node);

    else if (Node->tag == astFnImpl)
        analyzerFnImpl(ctx, Node);

    else if (Node->tag == astDecl)
        analyzerDecl(ctx, Node, false);

    else if (Node->tag == astCode)
        analyzerCode(ctx, Node);

    else if (Node->tag == astBranch)
        analyzerBranch(ctx, Node);

    else if (Node->tag == astLoop)
        analyzerLoop(ctx, Node);

    else if (Node->tag == astIter)
        analyzerIter(ctx, Node);

    else if (Node->tag == astReturn)
        analyzerReturn(ctx, Node);

    else if (Node->tag == astBreak || Node->tag == astContinue)
        ; /*Nothing to check (inside loop is a parsing issue)*/

    else if (astIsValueTag(Node->tag))
        /*TODO: Check not throwing away value*/
        analyzerValue(ctx, Node);

    else
        debugErrorUnhandled("analyzerNode", "AST tag", astTagGetStr(Node->tag));

    debugLeave();
}

static void analyzerModule (analyzerCtx* ctx, ast* Node) {
    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->tag == astUsing)
            analyzerUsing(ctx, Current);

        else if (Current->tag == astFnImpl)
            analyzerFnImpl(ctx, Current);

        else if (Current->tag == astDecl)
            analyzerDecl(ctx, Current, true);

        else
            debugErrorUnhandled("analyzerModule", "AST tag", astTagGetStr(Node->tag));
    }
}

static void analyzerUsing (analyzerCtx* ctx, ast* Node) {
    if (Node->r)
        analyzerNode(ctx, Node->r);
}

static void analyzerFnImpl (analyzerCtx* ctx, ast* Node) {
    /*Analyze the prototype*/

    analyzerDecl(ctx, Node->l, true);

    if (Node->symbol->tag != symId)
        errorFnTag(ctx, Node);

    else if (!typeIsFunction(Node->symbol->dt))
        errorTypeExpected(ctx, Node->l->firstChild, "implementation", "function");

    /*Analyze the implementation*/

    /*Save the old one, functions may be (illegally) nested*/
    analyzerFnCtx oldFnctx = analyzerPushFnctx(ctx, Node->symbol);

    analyzerNode(ctx, Node->r);

    analyzerPopFnctx(ctx, oldFnctx);
}

static void analyzerCode (analyzerCtx* ctx, ast* Node) {
    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        analyzerNode(ctx, Current);
}

static void analyzerBranch (analyzerCtx* ctx, ast* Node) {
    /*Is the condition a valid condition?*/

    ast* cond = Node->firstChild;
    analyzerValue(ctx, cond);

    if (!typeIsCondition(cond->dt))
        errorTypeExpected(ctx, cond, "if", "condition");

    /*Code*/

    analyzerNode(ctx, Node->l);

    if (Node->r)
        analyzerNode(ctx, Node->r);
}

static void analyzerLoop (analyzerCtx* ctx, ast* Node) {
    /*do while?*/
    bool isDo = Node->l->tag == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    /*Condition*/

    analyzerValue(ctx, cond);

    if (!typeIsCondition(cond->dt))
        errorTypeExpected(ctx, cond, "while loop", "condition");

    /*Code*/

    analyzerNode(ctx, code);
}

static void analyzerIter (analyzerCtx* ctx, ast* Node) {
    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    /*Initializer*/

    analyzerNode(ctx, init);

    /*Condition*/

    if (cond->tag != astEmpty) {
        analyzerValue(ctx, cond);

        if (!typeIsCondition(cond->dt))
            errorTypeExpected(ctx, cond, "for loop", "condition");
    }

    /*Iterator*/

    if (iter->tag != astEmpty)
        analyzerValue(ctx, iter);

    /*Code*/

    analyzerNode(ctx, Node->l);
}

static void analyzerReturn (analyzerCtx* ctx, ast* Node) {
    /*Return type, if any, matches?*/

    const type* R = Node->r ? analyzerValue(ctx, Node->r) : 0;

    if (ctx->fnctx.returnType) {
        if (R) {
            if (!typeIsCompatible(R, ctx->fnctx.returnType))
                errorReturnType(ctx, Node->r, ctx->fnctx);

        } else if (!typeIsVoid(ctx->fnctx.returnType))
            errorReturnType(ctx, Node->r, ctx->fnctx);

    /*No known return type because we're in a lambda:
       - Infer the type from the expression given,
       - Any further returns will be checked against this, and it will also be
         used for the type of the lambda itself.*/
    } else
        ctx->fnctx.returnType = R ? typeDeepDuplicate(R)
                                  : typeCreateBasic(ctx->types[builtinVoid]);
}

