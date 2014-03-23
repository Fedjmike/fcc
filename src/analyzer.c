#include "../inc/analyzer.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"
#include "../inc/architecture.h"

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

    intsetInit(&ctx->incompleteDeclIgnore, 17);
    intsetInit(&ctx->incompletePtrIgnore, 17);

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
    analyzerFnCtx old = ctx->fnctx;
    ctx->fnctx = (analyzerFnCtx) {Symbol, typeDeriveReturn(Symbol->dt)};
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
        analyzerDecl(ctx, Node);

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
}

static void analyzerModule (analyzerCtx* ctx, ast* Node) {
    debugEnter("Module");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerNode(ctx, Current);
        //debugWait();
    }

    debugLeave();
}

static void analyzerUsing (analyzerCtx* ctx, ast* Node) {
    debugEnter("Using");

    analyzerNode(ctx, Node->r);

    debugLeave();
}

static void analyzerFnImpl (analyzerCtx* ctx, ast* Node) {
    debugEnter("FnImpl");

    /*Analyze the prototype*/

    analyzerDecl(ctx, Node->l);

    if (!typeIsFunction(Node->symbol->dt))
        errorTypeExpected(ctx, Node->l->firstChild, "implementation", "function");

    /*Analyze the implementation*/

    /*Save the old one, functions may be (illegally) nested*/
    analyzerFnCtx oldFnctx = analyzerPushFnctx(ctx, Node->symbol);

    analyzerNode(ctx, Node->r);

    analyzerPopFnctx(ctx, oldFnctx);

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
    analyzerValue(ctx, cond);

    if (!typeIsCondition(cond->dt))
        errorTypeExpected(ctx, cond, "if", "condition");

    /*Code*/

    analyzerNode(ctx, Node->l);

    if (Node->r)
        analyzerNode(ctx, Node->r);

    debugLeave();
}

static void analyzerLoop (analyzerCtx* ctx, ast* Node) {
    debugEnter("Loop");

    /*do while?*/
    bool isDo = Node->l->tag == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    /*Condition*/

    analyzerValue(ctx, cond);

    if (!typeIsCondition(cond->dt))
        errorTypeExpected(ctx, cond, "do loop", "condition");

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

    if (init->tag == astDecl)
        analyzerNode(ctx, init);

    else if (init->tag != astEmpty)
        analyzerValue(ctx, init);

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

    debugLeave();
}

static void analyzerReturn (analyzerCtx* ctx, ast* Node) {
    debugEnter("Return");

    /*Return type, if any, matches?*/

    if (Node->r) {
        const type* R = analyzerValue(ctx, Node->r);

        if (!typeIsCompatible(R, ctx->fnctx.returnType))
            errorTypeExpectedType(ctx, Node->r, "return", ctx->fnctx.returnType);

    } else if (!typeIsVoid(ctx->fnctx.returnType)) {
        Node->dt = typeCreateBasic(ctx->types[builtinVoid]);
        errorTypeExpectedType(ctx, Node, "return statement", ctx->fnctx.returnType);
    }

    debugLeave();
}

