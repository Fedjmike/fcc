#include "../inc/analyzer.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"

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

analyzerResult analyzer (ast* Tree, sym** Types) {
    analyzerCtx* ctx = analyzerInit(Types);

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

    else if (Node->tag == astBreak)
        ; /*Nothing to check (inside breakable block is a parsing issue)*/

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

    (void) ctx, (void) Node;
    analyzerNode(ctx, Node->r);

    debugLeave();
}

static void analyzerFnImpl (analyzerCtx* ctx, ast* Node) {
    debugEnter("FnImpl");

    /*Analyze the prototype*/

    analyzerDecl(ctx, Node->l);

    if (!typeIsFunction(Node->l->firstChild->symbol->dt))
        errorTypeExpected(ctx, Node, "implementation", "function", Node->symbol->dt);

    /*Analyze the implementation*/

    /*Save the old one, functions may be (illegally) nested*/
    type* oldReturn = ctx->returnType;
    ctx->returnType = typeDeriveReturn(Node->symbol->dt);

    analyzerNode(ctx, Node->r);

    typeDestroy(ctx->returnType);
    ctx->returnType = oldReturn;

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
    valueResult condRes = analyzerValue(ctx, cond);

    if (!typeIsCondition(condRes.dt))
        errorTypeExpected(ctx, cond, "if", "condition", condRes.dt);

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

    valueResult condRes = analyzerValue(ctx, cond);

    if (!typeIsCondition(condRes.dt))
        errorTypeExpected(ctx, cond, "do loop", "condition", condRes.dt);

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
        valueResult condRes = analyzerValue(ctx, cond);

        if (!typeIsCondition(condRes.dt))
            errorTypeExpected(ctx, cond, "for loop", "condition", condRes.dt);
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
        valueResult R = analyzerValue(ctx, Node->r);

        if (!typeIsCompatible(R.dt, ctx->returnType))
            errorTypeExpectedType(ctx, Node->r, "return", ctx->returnType, R.dt);

    } else if (!typeIsVoid(ctx->returnType)) {
        type* tmp = typeCreateBasic(ctx->types[builtinVoid]);
        errorTypeExpectedType(ctx, Node, "return statement", ctx->returnType, tmp);
        typeDestroy(tmp);
    }

    debugLeave();
}

