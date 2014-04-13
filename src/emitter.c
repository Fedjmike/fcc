#include "../inc/emitter.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/architecture.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"
#include "../inc/reg.h"

#include "../inc/emitter-value.h"
#include "../inc/emitter-decl.h"

#include "string.h"
#include "stdlib.h"

static void emitterBranchOnValue (emitterCtx* ctx, irBlock* block, const ast* value,
                                 irBlock* ifTrue, irBlock* ifFalse)

static void emitterModule (emitterCtx* ctx, const ast* Node);

static void emitterFnImpl (emitterCtx* ctx, const ast* Node);
static void emitterCode (emitterCtx* ctx, const ast* Node);
static void emitterLine (emitterCtx* ctx, const ast* Node);

static void emitterReturn (emitterCtx* ctx, const ast* Node);

static void emitterBranch (emitterCtx* ctx, const ast* Node);
static void emitterLoop (emitterCtx* ctx, const ast* Node);
static void emitterIter (emitterCtx* ctx, const ast* Node);

static emitterCtx* emitterInit (const char* output, const architecture* arch) {
    emitterCtx* ctx = malloc(sizeof(emitterCtx));
    ctx->Asm = asmInit(output, arch);
    ctx->arch = arch;
    ctx->labelReturnTo = operandCreate(operandUndefined);
    ctx->labelBreakTo = operandCreate(operandUndefined);
    return ctx;
}

static void emitterEnd (emitterCtx* ctx) {
    asmEnd(ctx->Asm);
    free(ctx);
}

void emitter (const ast* Tree, const char* output, const architecture* arch) {
    emitterCtx* ctx = emitterInit(output, arch);
    asmFilePrologue(ctx->Asm);

    emitterModule(ctx, Tree);

    asmFileEpilogue(ctx->Asm);
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
            emitterDecl(ctx, Current);

        else if (Current->tag == astEmpty)
            debugMsg("Empty");

        else
            debugErrorUnhandled("emitterModule", "AST tag", astTagGetStr(Current->tag));
    }

    debugLeave();
}

static int emitterScopeAssignOffsets (const architecture* arch, sym* Scope, int offset) {
    for (int n = 0; n < Scope->children.length; n++) {
        sym* Symbol = vectorGet(&Scope->children, n);

        if (Symbol->tag == symScope)
            offset = emitterScopeAssignOffsets(arch, Symbol, offset);

        else if (Symbol->tag == symId) {
            offset -= typeGetSize(arch, Symbol->dt);
            Symbol->offset = offset;
            reportSymbol(Symbol);

        } else {}
    }

    return offset;
}

static void emitterFnImpl (emitterCtx* ctx, const ast* Node) {
    debugEnter("FnImpl");

    if (Node->symbol->label == 0)
        ctx->arch->symbolMangler(Node->symbol);

    /*Two words already on the stack:
      return ptr and saved base pointer*/
    int lastOffset = 2*ctx->arch->wordsize;

    /*Returning through temporary?*/
    if (typeGetSize(ctx->arch, typeGetReturn(Node->symbol->dt)) > ctx->arch->wordsize)
        lastOffset += ctx->arch->wordsize;

    /*Asign offsets to all the parameters*/
    for (int n = 0; n < Node->symbol->children.length; n++) {
        sym* Symbol = vectorGet(&Node->symbol->children, n);

        if (Symbol->tag != symParam)
            break;

        Symbol->offset = lastOffset;
        lastOffset += typeGetSize(ctx->arch, Symbol->dt);

        reportSymbol(Symbol);
    }

    /*Allocate stack space for all the auto variables
      Stack grows down, so the amount is the negation of the last offset*/
    int stacksize = -emitterScopeAssignOffsets(ctx->arch, Node->symbol, 0);

    /*Label to jump to from returns*/
    operand EndLabel = ctx->labelReturnTo = asmCreateLabel(ctx->Asm, labelReturn);

    asmComment(ctx->Asm, "");
    asmFnPrologue(ctx->Asm, Node->symbol->label, stacksize);

    emitterCode(ctx, Node->r);
    asmFnEpilogue(ctx->Asm, EndLabel);

    debugLeave();
}

static void emitterCode (emitterCtx* ctx, const ast* Node) {
    asmEnter(ctx->Asm);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        emitterLine(ctx, Current);
    }

    asmLeave(ctx->Asm);
}

static void emitterLine (emitterCtx* ctx, const ast* Node) {
    debugEnter("Line");

    asmComment(ctx->Asm, "");

    if (Node->tag == astBranch)
        emitterBranch(ctx, Node);

    else if (Node->tag == astLoop)
        emitterLoop(ctx, Node);

    else if (Node->tag == astIter)
        emitterIter(ctx, Node);

    else if (Node->tag == astCode)
        emitterCode(ctx, Node);

    else if (Node->tag == astReturn)
        emitterReturn(ctx, Node);

    else if (Node->tag == astBreak)
        asmJump(ctx->Asm, ctx->labelBreakTo);

    else if (Node->tag == astContinue)
        asmJump(ctx->Asm, ctx->labelContinueTo);

    else if (Node->tag == astDecl)
        emitterDecl(ctx, Node);

    else if (astIsValueTag(Node->tag))
        operandFree(emitterValue(ctx, Node, requestAny));

    else if (Node->tag == astEmpty)
        debugMsg("Empty");

    else
        debugErrorUnhandled("emitterLine", "AST tag", astTagGetStr(Node->tag));

    debugLeave();
}

static void emitterReturn (emitterCtx* ctx, const ast* Node) {
    debugEnter("Return");

    /*Non void return?*/
    if (Node->r) {
        operand Ret = emitterValue(ctx, Node->r, requestOperable);
        int retSize = typeGetSize(ctx->arch, Node->r->dt);

        bool retInTemp = retSize > ctx->arch->wordsize;

        /*Larger than word size ret => copy into caller allocated temporary pushed after args*/
        if (retInTemp) {
            operand tempRef = operandCreateReg(regAlloc(ctx->arch->wordsize));

            /*Dereference the temporary*/
            asmMove(ctx->Asm, tempRef, operandCreateMem(&regs[regRBP], 2*ctx->arch->wordsize, ctx->arch->wordsize));
            /*Copy over the value*/
            asmMove(ctx->Asm, operandCreateMem(tempRef.base, 0, retSize), Ret);
            operandFree(Ret);

            /*Return the temporary reference*/
            Ret = tempRef;
        }

        reg* rax;

        /*Returning either the return value itself or a reference to it*/
        if ((rax = regRequest(regRAX, retInTemp ? ctx->arch->wordsize : retSize)) != 0) {
            asmMove(ctx->Asm, operandCreateReg(rax), Ret);
            regFree(rax);

        } else if (Ret.base != regGet(regRAX))
            debugError("emitterLine", "unable to allocate RAX for return");

        operandFree(Ret);
    }

    asmJump(ctx->Asm, ctx->labelReturnTo);

    debugLeave();
}

static void emitterBranchOnValue (emitterCtx* ctx, irBlock* block, const ast* value,
                                 irBlock* ifTrue, irBlock* ifFalse) {
    operand cond = emitterValue(ctx, &block, value, requestFlags);
    irBranch(block, cond, ifTrue, ifFalse);
}

static void emitterBranch (emitterCtx* ctx, irBlock* block, const ast* Node, irBlock* continuation) {
    debugEnter("Branch");

    irBlock* ifTrue = irBlockCreate(ctx->ir),
             ifFalse = irBlockCreate(ctx->ir);

    /*Condition, branch*/
    emitterBranchOnValue(ctx, block, Node->firstChild, ifTrue, ifFalse);

    /*Emit the true and false branches*/
    emitterCode(ctx, ifTrue, Node->l, continuation);
    emitterCode(ctx, ifFalse, Node->r, continuation);

    debugLeave();
}

static void emitterLoop (emitterCtx* ctx, irBlock* block, const ast* Node, irBlock* continuation) {
    irBlock* body = irBlockCreate(ctx->ir),
             loopCheck = irBlockCreate(ctx->ir);

    /*Work out which order the condition and code came in
      => whether this is a while or a do while*/
    bool isDo = Node->l->tag == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    /*A do while, no initial condition*/
    if (isDo)
        irJump(body, code);

    /*Initial condition: go into the body, or exit to the continuation*/
    else
        emitterBranchOnValue(ctx, &block, cond, body, continuation);

    /*Loop body*/

    irBlock* oldBreakTo = emitterSetBreakTo(ctx, continuation);
    irBlock* oldContinueTo = emitterSetContinueTo(ctx, loopCheck);

    emitterCode(ctx, body, code, loopCheck);

    ctx->breakTo = oldBreakTo;
    ctx->continueTo = oldContinueTo;

    /*Loop re-entrant condition (in the loopCheck block this time)*/
    emitterBranchOnValue(ctx, &loopCheck, cond, body, continuation);
}

static void emitterIter (emitterCtx* ctx, const ast* Node, irBlock* continuation) {
    irBlock* body = irBlockCreate(ctx->ir),
             iterate = irBlockCreate(ctx->ir);

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    /*Initialization*/

    if (init->tag == astDecl)
        emitterDecl(ctx, init);

    else
        emitterValue(ctx, &block, init, requestVoid);

    /*Condition*/
    emitterBranchOnValue(ctx, &block, cond, body, continuation);

    /*Body*/

    irBlock* oldBreakTo = emitterSetBreakTo(ctx, continuation);
    irBlock* oldContinueTo = emitterSetContinueTo(ctx, iterate);

    emitterCode(ctx, body, code, iterate);

    ctx->breakTo = oldBreakTo;
    ctx->continueTo = oldContinueTo;

    /*Iterate and loop check*/
    emitterValue(Ctx, &iterate, iter, requestVoid);
    emitterBranchOnValue(ctx, iterate, cond, body, continuation);
}
