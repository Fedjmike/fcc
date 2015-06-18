#include "../inc/emitter-internal.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"
#include "../inc/ir.h"
#include "../inc/reg.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "stdlib.h"

static int emitterScopeAssignOffsets (const architecture* arch, sym* Scope, int offset);

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
    if (regIsUsed(r))
        asmSaveReg(ctx->ir, block, r);

    *oldSize = regs[r].allocatedAs;
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
    if (value->tag == astEmpty)
        irJump(block, ifTrue);

    else {
        operand cond = emitterValue(ctx, &block, value, requestFlags);
        irBranch(block, cond, ifTrue, ifFalse);
    }
}

operand emitterWiden (emitterCtx* ctx, irBlock* block, operand R, int size) {
    (void) ctx;

    if (R.tag == operandLiteral)
        return R;

    char* RStr = operandToStr(R);

    operand L;

    if (R.tag == operandReg) {
        R.base->allocatedAs = size;
        L = R;

    } else
        L = operandCreateReg(regAlloc(size));

    char* LStr = operandToStr(L);
    irBlockOut(block, "movsx %s, %s", LStr, RStr);
    free(LStr);
    free(RStr);

    if (R.tag != operandReg)
        operandFree(R);

    return L;
}

operand emitterNarrow (emitterCtx* ctx, irBlock* block, operand R, int size) {
    if (R.tag == operandLiteral)
        return R;

    operand L = emitterGetInReg(ctx, block, R, operandGetSize(ctx->arch, R));
    L.base->allocatedAs = size;
    return L;
}

void emitterZeroMem (emitterCtx* ctx, irBlock* block, operand L) {
    int size = operandGetSize(ctx->arch, L);

    operand zero = operandCreateLiteral(0);

    int regPressure =   (regIsUsed(regRAX) ? 1 : 0)
                      + (regIsUsed(regRCX) ? 1 : 0)
                      + (regIsUsed(regRDI) ? 1 : 0);

    if (size >= ctx->arch->wordsize*10*(1+regPressure)) {
        int raxOldSize, rcxOldSize, rdxOldSize;
        operand RAX = emitterTakeReg(ctx, block, regRAX, &raxOldSize, ctx->arch->wordsize);
        operand RCX = emitterTakeReg(ctx, block, regRCX, &rcxOldSize, ctx->arch->wordsize);
        operand RDI = emitterTakeReg(ctx, block, regRDI, &rdxOldSize, ctx->arch->wordsize);

        int excess = size % ctx->arch->wordsize;
        asmRepStos(ctx->ir, block, RAX, RCX, RDI, L, size-excess, zero);

        if (excess != 0)
            asmMove(ctx->ir, block, operandCreateMem(RDI.base, 0, excess), zero);

        emitterGiveBackReg(ctx, block, regRAX, raxOldSize);
        emitterGiveBackReg(ctx, block, regRCX, rcxOldSize);
        emitterGiveBackReg(ctx, block, regRDI, rdxOldSize);

    } else {
        int chunk = ctx->arch->wordsize;
        L.size = chunk;

        for (int i = 0;
             i+chunk <= size;
             i += chunk, L.offset += chunk)
            asmMove(ctx->ir, block, L, zero);

        if (size % chunk != 0) {
            L.size = size % chunk;
            asmMove(ctx->ir, block, L, zero);
        }
    }
}
