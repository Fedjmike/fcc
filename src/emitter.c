#include "../inc/emitter.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"
#include "../inc/reg.h"

#include "../inc/emitter-value.h"
#include "../inc/emitter-decl.h"

#include "string.h"
#include "stdlib.h"

static void emitterModule (emitterCtx* ctx, const ast* Tree);

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

    labelFreeAll();

    asmFileEpilogue(ctx->Asm);
    emitterEnd(ctx);
}

static void emitterModule (emitterCtx* ctx, const ast* Tree) {
    debugEnter("Module");

    for (ast* Current = Tree->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->tag == astUsing)
            emitterModule(ctx, Current->r);

        else if (Current->tag == astFnImpl)
            emitterFnImpl(ctx, Current);

        else if (Current->tag == astDecl)
            emitterDecl(ctx, Current);

        else
            debugErrorUnhandled("emitterModule", "AST tag", astTagGetStr(Current->tag));
    }

    debugLeave();
}

static int emitterScopeAssignOffsets (const architecture* arch, sym* Scope, int offset) {
    for (sym* Symbol = Scope->firstChild;
         Symbol;
         Symbol = Symbol->nextSibling) {
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

    Node->symbol->label = labelNamed(Node->symbol->ident);

    /*Two words already on the stack:
      return ptr and saved base pointer*/
    int lastOffset = 2*ctx->arch->wordsize;
    sym* Symbol;

    /*Returning through temporary?*/
    if (typeGetSize(ctx->arch, Node->symbol->dt->returnType) > ctx->arch->wordsize)
        lastOffset += ctx->arch->wordsize;

    /*Asign offsets to all the parameters*/
    for (Symbol = Node->symbol->firstChild;
         Symbol && Symbol->tag == symParam;
         Symbol = Symbol->nextSibling) {
        Symbol->offset = lastOffset;
        lastOffset += typeGetSize(ctx->arch, Symbol->dt);

        reportSymbol(Symbol);
    }

    /*Allocate stack space for all the auto variables
      Stack grows down, so the amount is the negation of the last offset*/
    int stacksize = -emitterScopeAssignOffsets(ctx->arch, Node->symbol, 0);

    /*Label to jump to from returns*/
    operand EndLabel = ctx->labelReturnTo = labelCreate(labelReturn);

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

    else if (Node->tag == astReturn)
        emitterReturn(ctx, Node);

    else if (Node->tag == astBreak)
        asmJump(ctx->Asm, ctx->labelBreakTo);

    else if (Node->tag == astContinue)
        asmJump(ctx->Asm, ctx->labelContinueTo);

    else if (Node->tag == astDecl)
        emitterDecl(ctx, Node);

    else if (astIsValueTag(Node->tag))
        operandFree(emitterValue(ctx, Node, operandCreate(operandUndefined)));

    else
        debugErrorUnhandled("emitterLine", "AST tag", astTagGetStr(Node->tag));

    debugLeave();
}

static void emitterReturn (emitterCtx* ctx, const ast* Node) {
    debugEnter("Return");

    /*Non void return?*/
    if (Node->r) {
        operand Ret = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        int retSize = typeGetSize(ctx->arch, Node->r->dt);

        bool retInTemp = retSize > ctx->arch->wordsize;

        /*Larger than word size ret => copy into caller allocated temporary pushed after args*/
        if (retInTemp) {
            operand tempRef = operandCreateReg(regAlloc(ctx->arch->wordsize));

            /*Dereference the temporary*/
            asmMove(ctx->Asm, tempRef, operandCreateMem(&regs[regRBP], 2*ctx->arch->wordsize, ctx->arch->wordsize));
            /*Copy over the value*/
            asmMove(ctx->Asm, operandCreateMem(tempRef.reg, 0, retSize), Ret);
            operandFree(Ret);

            /*Return the temporary reference*/
            Ret = tempRef;
        }

        reg* rax;

        /*Returning either the return value itself or a reference to it*/
        if ((rax = regRequest(regRAX, retInTemp ? ctx->arch->wordsize : retSize)) != 0) {
            asmMove(ctx->Asm, operandCreateReg(rax), Ret);
            regFree(rax);

        } else if (Ret.reg != &regs[regRAX])
            debugError("emitterLine", "unable to allocate RAX for return");

        operandFree(Ret);
    }

    asmJump(ctx->Asm, ctx->labelReturnTo);

    debugLeave();
}

static void emitterBranch (emitterCtx* ctx, const ast* Node) {
    debugEnter("Branch");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);

    /*Compute the condition, requesting it be placed in the flags*/
    asmBranch(ctx->Asm,
              emitterValue(ctx,
                           Node->firstChild,
                           operandCreateFlags(conditionUndefined)),
              ElseLabel);

    emitterCode(ctx, Node->l);

    if (Node->r) {
        asmComment(ctx->Asm, "");
        asmJump(ctx->Asm, EndLabel);
        asmLabel(ctx->Asm, ElseLabel);

        emitterCode(ctx, Node->r);

        asmLabel(ctx->Asm, EndLabel);

    } else
        asmLabel(ctx->Asm, ElseLabel);

    debugLeave();
}

static void emitterLoop (emitterCtx* ctx, const ast* Node) {
    debugEnter("Loop");

    /*The place to return to loop again (after confirming condition)*/
    operand LoopLabel = labelCreate(labelUndefined);

    operand OldBreakTo = ctx->labelBreakTo;
    operand OldContinueTo = ctx->labelContinueTo;
    operand EndLabel = ctx->labelBreakTo = labelCreate(labelUndefined);
    ctx->labelContinueTo = labelCreate(labelUndefined);

    /*Work out which order the condition and code came in
      => whether this is a while or a do while*/
    bool isDo = Node->l->tag == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    /*Condition*/

    if (!isDo)
        asmBranch(ctx->Asm,
                  emitterValue(ctx, cond, operandCreateFlags(conditionUndefined)),
                  EndLabel);

    /*Code*/

    asmLabel(ctx->Asm, LoopLabel);
    emitterCode(ctx, code);

    asmComment(ctx->Asm, "");

    /*Condition*/

    asmLabel(ctx->Asm, ctx->labelContinueTo);

    asmBranch(ctx->Asm,
              emitterValue(ctx, cond, operandCreateFlags(conditionUndefined)),
              EndLabel);

    asmJump(ctx->Asm, LoopLabel);
    asmLabel(ctx->Asm, EndLabel);

    ctx->labelBreakTo = OldBreakTo;
    ctx->labelContinueTo = OldContinueTo;

    debugLeave();
}

static void emitterIter (emitterCtx* ctx, const ast* Node) {
    debugEnter("Iter");

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    operand LoopLabel = labelCreate(labelUndefined);

    operand OldBreakTo = ctx->labelBreakTo;
    operand OldContinueTo = ctx->labelContinueTo;
    operand EndLabel = ctx->labelBreakTo = labelCreate(labelUndefined);
    ctx->labelContinueTo = labelCreate(labelUndefined);

    /*Initialize stuff*/

    if (init->tag == astDecl) {
        emitterDecl(ctx, init);
        asmComment(ctx->Asm, "");

    } else if (astIsValueTag(init->tag)) {
        operandFree(emitterValue(ctx, init, operandCreate(operandUndefined)));
        asmComment(ctx->Asm, "");

    } else if (init->tag != astEmpty)
        debugErrorUnhandled("emitterIter", "AST tag", astTagGetStr(init->tag));


    /*Check condition*/

    asmLabel(ctx->Asm, LoopLabel);

    if (cond->tag != astEmpty) {
        operand Condition = emitterValue(ctx, cond, operandCreateFlags(conditionUndefined));
        asmBranch(ctx->Asm, Condition, EndLabel);
    }

    /*Do block*/

    emitterCode(ctx, Node->l);
    asmComment(ctx->Asm, "");

    /*Iterate*/

    asmLabel(ctx->Asm, ctx->labelContinueTo);

    if (iter->tag != astEmpty) {
        operandFree(emitterValue(ctx, iter, operandCreate(operandUndefined)));
        asmComment(ctx->Asm, "");
    }

    /*loopen Sie*/

    asmJump(ctx->Asm, LoopLabel);
    asmLabel(ctx->Asm, EndLabel);

    ctx->labelBreakTo = OldBreakTo;
    ctx->labelContinueTo = OldContinueTo;

    debugLeave();
}
