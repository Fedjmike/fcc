#include "../inc/emitter-helpers.h"

#include "../inc/ir.h"
#include "../inc/reg.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "../inc/emitter.h"
#include "../inc/emitter-value.h"

#include "stdlib.h"

irFn* emitterSetFn (emitterCtx* ctx, irFn* fn) {
    irFn* old = ctx->curFn;
    ctx->curFn = fn;
    return old;
}

irBlock* emitterSetReturnTo (emitterCtx* ctx, irBlock* block) {
    irBlock* old = ctx->returnTo;
    ctx->returnTo = block;
    return old;
}

irBlock* emitterSetBreakTo (emitterCtx* ctx, irBlock* block) {
    irBlock* old = ctx->breakTo;
    ctx->breakTo = block;
    return old;
}

irBlock* emitterSetContinueTo (emitterCtx* ctx, irBlock* block) {
    irBlock* old = ctx->continueTo;
    ctx->continueTo = block;
    return old;
}

int emitterFnAllocateStack (const architecture* arch, sym* fn) {
    /*Two words already on the stack:
      return ptr and saved base pointer*/
    int lastOffset = 2*arch->wordsize;

    /*Returning through temporary?*/
    if (typeGetSize(arch, typeGetReturn(fn->dt)) > arch->wordsize)
        lastOffset += arch->wordsize;

    /*Assign offsets to all the parameters*/
    for (int n = 0; n < fn->children.length; n++) {
        sym* param = vectorGet(&fn->children, n);

        if (param->tag != symParam)
            break;

        param->offset = lastOffset;
        lastOffset += typeGetSize(arch, param->dt);

        reportSymbol(param);
    }

    /*Allocate stack space for all the auto variables
      Stack grows down, so the amount is the negation of the last offset*/
    return -emitterScopeAssignOffsets(arch, fn, 0);
}

operand emitterGetInReg (emitterCtx* ctx,  irBlock* block, operand src, int size) {
    if (src.tag == operandReg)
        return src;

    operand dest = operandCreateReg(regAlloc(size));
    asmMove(ctx->ir, block, dest, src);
    operandFree(src);
    return dest;
}

operand emitterTakeReg (emitterCtx* ctx, irBlock* block, regIndex r, int* oldSize, int newSize) {
    if (regIsUsed(r)) {
        *oldSize = regs[r].allocatedAs;
        asmSaveReg(ctx->ir, block, r);

    } else
        *oldSize = 0;

    regs[r].allocatedAs = newSize;
    return operandCreateReg(&regs[r]);
}

void emitterGiveBackReg (emitterCtx* ctx, irBlock* block, regIndex r, int oldSize) {
    regs[r].allocatedAs = oldSize;

    if (oldSize)
        asmRestoreReg(ctx->ir, block, r);
}

void emitterBranchOnValue (emitterCtx* ctx, irBlock* block, const ast* value,
                           irBlock* ifTrue, irBlock* ifFalse) {
    operand cond = emitterValue(ctx, &block, value, requestFlags);
    irBranch(block, cond, ifTrue, ifFalse);
}

operand emitterWiden (emitterCtx* ctx, irBlock* block, operand R, int size) {
    (void) block;
    operand L;

    if (R.tag == operandReg) {
        R.base->allocatedAs = size;
        L = R;

    } else
        L = operandCreateReg(regAlloc(size));

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);
    asmOutLn(ctx->ir->asm, "movsx %s, %s", LStr, RStr);
    free(LStr);
    free(RStr);

    if (R.tag != operandReg)
        operandFree(R);

    return L;
}

operand emitterNarrow (emitterCtx* ctx, irBlock* block, operand R, int size) {
    operand L = emitterGetInReg(ctx, block, R, operandGetSize(ctx->arch, R));
    L.base->allocatedAs = size;
    return L;
}
