#include "../inc/emitter.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/architecture.h"
#include "../inc/ir.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"
#include "../inc/reg.h"

#include "../inc/emitter-value.h"
#include "../inc/emitter-decl.h"
#include "../inc/emitter-helpers.h"

#include "string.h"
#include "stdlib.h"

using "../inc/emitter.h";

using "../std/std.h";

using "../inc/debug.h";
using "../inc/type.h";
using "../inc/ast.h";
using "../inc/sym.h";
using "../inc/architecture.h";
using "../inc/ir.h";
using "../inc/operand.h";
using "../inc/asm.h";
using "../inc/asm-amd64.h";
using "../inc/reg.h";

using "../inc/emitter-value.h";
using "../inc/emitter-decl.h";
using "../inc/emitter-helpers.h";

using "string.h";
using "stdlib.h";

static void emitterModule (emitterCtx* ctx, const ast* Node);
static void emitterFnImpl (emitterCtx* ctx, const ast* Node);

static irBlock* emitterLine (emitterCtx* ctx, irBlock* block, const ast* Node);

static void emitterReturn (emitterCtx* ctx, irBlock* block, const ast* Node);

static irBlock* emitterBranch (emitterCtx* ctx, irBlock* block, const ast* Node);
static irBlock* emitterLoop (emitterCtx* ctx, irBlock* block, const ast* Node);
static irBlock* emitterIter (emitterCtx* ctx, irBlock* block, const ast* Node);

static emitterCtx* emitterInit (const char* output, const architecture* arch) {
    emitterCtx* ctx = malloc(sizeof(emitterCtx));
    ctx->ir = malloc(sizeof(irCtx));
    irInit(ctx->ir, output, arch);
    ctx->arch = arch;
    ctx->returnTo = 0;
    ctx->breakTo = 0;
    ctx->continueTo = 0;
    return ctx;
}

static void emitterEnd (emitterCtx* ctx) {
    irFree(ctx->ir);

    free(ctx->ir);
    free(ctx);
}

void emitter (const ast* Tree, const char* output, const architecture* arch) {
    emitterCtx* ctx = emitterInit(output, arch);

    emitterModule(ctx, Tree);

    irBlockLevelAnalysis(ctx->ir);
    irEmit(ctx->ir);

    emitterEnd(ctx);
}

static void emitterModule (emitterCtx* ctx, const ast* Node) {
    debugEnter("Module");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->tag == astUsing) {
            if (Current->r)
                emitterModule(ctx, Current->r);

        } else if (Current->tag == astFnImpl)
            emitterFnImpl(ctx, Current);

        else if (Current->tag == astDecl)
            emitterDecl(ctx, 0, Current);

        else if (Current->tag == astEmpty)
            debugMsg("Empty");

        else
            debugErrorUnhandled("emitterModule", "AST tag", astTagGetStr(Current->tag));
    }

    debugLeave();
}

static void emitterFnImpl (emitterCtx* ctx, const ast* Node) {
    debugEnter("FnImpl");

    emitterDecl(ctx, 0, Node->l);

    int stacksize = emitterFnAllocateStack(ctx->arch, Node->symbol);

    /* */
    irFn* fn = irFnCreate(ctx->ir, Node->symbol->label, stacksize);
    ctx->curFn = fn;
    ctx->returnTo = fn->epilogue;

    emitterCode(ctx, fn->entryPoint, Node->r, fn->epilogue);

    debugLeave();
}

irBlock* emitterCode (emitterCtx* ctx, irBlock* block, const ast* Node, irBlock* continuation) {
    if (Node)
        for (ast* Current = Node->firstChild;
             Current;
             Current = Current->nextSibling)
            block = emitterLine(ctx, block, Current);

    irJump(block, continuation);

    return continuation;
}

static irBlock* emitterLine (emitterCtx* ctx, irBlock* block, const ast* Node) {
    debugEnter(astTagGetStr(Node->tag));

    if (Node->tag == astBranch)
        block = emitterBranch(ctx, block, Node);

    else if (Node->tag == astLoop)
        block = emitterLoop(ctx, block, Node);

    else if (Node->tag == astIter)
        block = emitterIter(ctx, block, Node);

    else if (Node->tag == astCode)
        block = emitterCode(ctx, block, Node, irBlockCreate(ctx->ir, ctx->curFn));

    else if (Node->tag == astReturn)
        emitterReturn(ctx, block, Node);

    else if (Node->tag == astBreak)
        irJump(block, ctx->breakTo);

    else if (Node->tag == astContinue)
        irJump(block, ctx->continueTo);

    else if (Node->tag == astDecl)
        emitterDecl(ctx, &block, Node);

    else if (astIsValueTag(Node->tag))
        emitterValue(ctx, &block, Node, requestVoid);

    else if (Node->tag == astEmpty)
        ;

    else
        debugErrorUnhandled("emitterLine", "AST tag", astTagGetStr(Node->tag));

    /*If the current block is terminated then this line was a return, break or continue.
      Any code following this is dead, but give it a block to put it in anyway.*/
    if (block->term)
        block = irBlockCreate(ctx->ir, ctx->curFn);

    debugLeave();

    return block;
}

static void emitterReturn (emitterCtx* ctx, irBlock* block, const ast* Node) {
    /*Non void return?*/
    if (Node->r)
        emitterValue(ctx, &block, Node->r, requestReturn);

    irJump(block, ctx->returnTo);
}

static irBlock* emitterBranch (emitterCtx* ctx, irBlock* block, const ast* Node) {
    irBlock *ifTrue = irBlockCreate(ctx->ir, ctx->curFn),
            *ifFalse = irBlockCreate(ctx->ir, ctx->curFn),
            *continuation = irBlockCreate(ctx->ir, ctx->curFn);

    /*Condition, branch*/
    emitterBranchOnValue(ctx, block, Node->firstChild, ifTrue, ifFalse);

    /*Emit the true and false branches*/
    emitterCode(ctx, ifTrue, Node->l, continuation);
    emitterCode(ctx, ifFalse, Node->r, continuation);

    return continuation;
}

static irBlock* emitterLoop (emitterCtx* ctx, irBlock* block, const ast* Node) {
    irBlock *body = irBlockCreate(ctx->ir, ctx->curFn),
            *loopCheck = irBlockCreate(ctx->ir, ctx->curFn),
            *continuation = irBlockCreate(ctx->ir, ctx->curFn);

    /*Work out which order the condition and code came in
      => whether this is a while or a do while*/
    bool isDo = Node->l->tag == astCode;
    ast *cond = isDo ? Node->r : Node->l,
        *code = isDo ? Node->l : Node->r;

    /*A do while, no initial condition*/
    if (isDo)
        irJump(block, body);

    /*Initial condition: go into the body, or exit to the continuation*/
    else
        emitterBranchOnValue(ctx, block, cond, body, continuation);

    /*Loop body*/

    irBlock *oldBreakTo = emitterSetBreakTo(ctx, continuation),
            *oldContinueTo = emitterSetContinueTo(ctx, loopCheck);

    emitterCode(ctx, body, code, loopCheck);

    ctx->breakTo = oldBreakTo;
    ctx->continueTo = oldContinueTo;

    /*Loop re-entrant condition (in the loopCheck block this time)*/
    emitterBranchOnValue(ctx, loopCheck, cond, body, continuation);

    return continuation;
}

static irBlock* emitterIter (emitterCtx* ctx, irBlock* block, const ast* Node) {
    irBlock *body = irBlockCreate(ctx->ir, ctx->curFn),
            *iterate = irBlockCreate(ctx->ir, ctx->curFn),
            *continuation = irBlockCreate(ctx->ir, ctx->curFn);

    ast *init = Node->firstChild,
        *cond = init->nextSibling,
        *iter = cond->nextSibling,
        *code = Node->l;

    /*Initialization*/

    if (init->tag == astDecl)
        emitterDecl(ctx, &block, init);

    else
        emitterValue(ctx, &block, init, requestVoid);

    /*Condition*/

    emitterBranchOnValue(ctx, block, cond, body, continuation);

    /*Body*/

    irBlock* oldBreakTo = emitterSetBreakTo(ctx, continuation);
    irBlock* oldContinueTo = emitterSetContinueTo(ctx, iterate);

    emitterCode(ctx, body, code, iterate);

    ctx->breakTo = oldBreakTo;
    ctx->continueTo = oldContinueTo;

    /*Iterate and loop check*/

    emitterValue(ctx, &iterate, iter, requestVoid);
    emitterBranchOnValue(ctx, iterate, cond, body, continuation);

    return continuation;
}
