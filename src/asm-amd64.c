#include "../inc/asm-amd64.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/asm.h"
#include "../inc/reg.h"

#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

void asmComment (asmCtx* ctx, char* str) {
    asmOutLn(ctx, ";%s", str);
}

void asmFilePrologue (asmCtx* ctx) {
    asmOutLn(ctx, ".file 1 \"%s\"", ctx->filename);
    asmOutLn(ctx, ".intel_syntax noprefix");
}

void asmFileEpilogue (asmCtx* ctx) {
    (void) ctx;
}

void asmFnPrologue (asmCtx* ctx, operand name, int localSize) {
    /*Symbol, linkage and alignment*/
    asmOutLn(ctx, ".balign 16");
    asmOutLn(ctx, ".globl %s", labelGet(name));
    asmOutLn(ctx, "%s:", labelGet(name));

    /*Register saving, create a new stack frame, stack variables etc*/

    asmPush(ctx, ctx->basePtr);
    asmMove(ctx, ctx->basePtr, ctx->stackPtr);

    if (localSize != 0)
        asmBOP(ctx, bopSub, ctx->stackPtr, operandCreateLiteral(localSize));

    for (int i = 0; i < ctx->arch->scratchRegs.length; i++) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->scratchRegs, i);
        asmSaveReg(ctx, r);
    }
}

void asmFnEpilogue (asmCtx* ctx, operand labelEnd) {
    /*Exit stack frame*/
    asmOutLn(ctx, "%s:", labelGet(labelEnd));

    /*Pop off saved regs in reverse order*/
    for (int i = ctx->arch->scratchRegs.length-1; i >= 0 ; i--) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->scratchRegs, i);
        asmRestoreReg(ctx, r);
    }

    asmMove(ctx, ctx->stackPtr, ctx->basePtr);
    asmPop(ctx, ctx->basePtr);
    asmOutLn(ctx, "ret");
}

void asmSaveReg (asmCtx* ctx, regIndex r) {
    asmOutLn(ctx, "push %s", regGetName(r, ctx->arch->wordsize));
}

void asmRestoreReg (asmCtx* ctx, regIndex r) {
    asmOutLn(ctx, "pop %s", regGetName(r, ctx->arch->wordsize));
}

void asmStringConstant (struct asmCtx* ctx, operand label, const char* str) {
    asmOutLn(ctx, ".section .rodata");
    asmOutLn(ctx, "%s:", labelGet(label));
    asmOutLn(ctx, ".ascii \"%s\\0\"", str);    /* .ascii "%s\0" */
    asmOutLn(ctx, ".section .text");
}

void asmLabel (asmCtx* ctx, operand L) {
    asmOutLn(ctx, "%s:", labelGet(L));
}

void asmJump (asmCtx* ctx, operand L) {
    if (L.tag == operandLabel)
        asmOutLn(ctx, "jmp %s", labelGet(L));

    else {
        char* LStr = operandToStr(L);
        asmOutLn(ctx, "jmp %s", LStr);
        free(LStr);
    }
}

void asmBranch (asmCtx* ctx, operand Condition, operand L) {
    char* CStr = operandToStr(Condition);

    if (L.tag == operandLabel)
        asmOutLn(ctx, "j%s %s", CStr, labelGet(L));

    else {
        char* LStr = operandToStr(L);
        asmOutLn(ctx, "j%s %s", CStr, LStr);
        free(LStr);
    }

    free(CStr);
}

void asmCall (asmCtx* ctx, operand L) {
    if (L.tag == operandLabel)
        asmOutLn(ctx, "call %s", labelGet(L));

    else {
        char* LStr = operandToStr(L);
        asmOutLn(ctx, "call %s", LStr);
        free(LStr);
    }
}

void asmPush (asmCtx* ctx, operand L) {
    /*Flags*/
    if (L.tag == operandFlags) {
        asmPush(ctx, operandCreateLiteral(1));
        operand top = operandCreateMem(ctx->stackPtr.base, 0, ctx->arch->wordsize);
        asmConditionalMove(ctx, L, top, operandCreateLiteral(0));

    /*Larger than word*/
    } else if (operandGetSize(ctx->arch, L) > ctx->arch->wordsize) {
        debugAssert("asmPush", "memory operand", L.tag == operandMem);

        int size = operandGetSize(ctx->arch, L);

        /*Push on *backwards* in word chunks.
          Start at the highest address*/
        L.offset += size;
        L.size = ctx->arch->wordsize;

        for (int i = 0; i < size; i += ctx->arch->wordsize) {
            L.offset -= ctx->arch->wordsize;
            asmPush(ctx, L);
        }

    /*Smaller than a word*/
    } else if (L.tag == operandMem && operandGetSize(ctx->arch, L) < ctx->arch->wordsize) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmMove(ctx, intermediate, L);
        asmPush(ctx, intermediate);
        operandFree(intermediate);

    } else {
        char* LStr = operandToStr(L);
        asmOutLn(ctx, "push %s", LStr);
        free(LStr);
    }
}

void asmPop (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "pop %s", LStr);
    free(LStr);
}

void asmPushN (asmCtx* ctx, int n) {
    if (n)
        asmBOP(ctx, bopSub, ctx->stackPtr, operandCreateLiteral(n*ctx->arch->wordsize));
}

void asmPopN (asmCtx* ctx, int n) {
    if (n)
        asmBOP(ctx, bopAdd, ctx->stackPtr, operandCreateLiteral(n*ctx->arch->wordsize));
}

void asmMove (asmCtx* ctx, operand Dest, operand Src) {
    if (Dest.tag == operandInvalid || Src.tag == operandInvalid)
        return;

    /*Too big for single register*/
    else if (operandGetSize(ctx->arch, Dest) > ctx->arch->wordsize) {
        debugAssert("asmMove", "Dest mem", Dest.tag == operandMem);
        debugAssert("asmMove", "Src mem", Src.tag == operandMem);
        debugAssert("asmMove", "operand size equality",
                    operandGetSize(ctx->arch, Dest) == operandGetSize(ctx->arch, Src));

        int size = operandGetSize(ctx->arch, Dest);
        int chunk = ctx->arch->wordsize;
        Dest.size = Src.size = chunk;

        /*Move up the operands, so far as a chunk would not go past their ends*/
        for (int i = 0;
             i+chunk <= size;
             i += chunk, Dest.offset += chunk, Src.offset += chunk)
            asmMove(ctx, Dest, Src);

        /*Were the operands not an even multiple of the chunk size?*/
        if (size % chunk != 0) {
            /*Final chunk size is the remainder*/
            Dest.size = Src.size = size % chunk;
            asmMove(ctx, Dest, Src);
        }

    /*Both memory operands*/
    } else if (Dest.tag == operandMem && Src.tag == operandMem) {
        operand intermediate = operandCreateReg(regAlloc(max(Dest.size, Src.size)));
        asmMove(ctx, intermediate, Src);
        asmMove(ctx, Dest, intermediate);
        operandFree(intermediate);

    /*Flags*/
    } else if (Src.tag == operandFlags) {
        asmMove(ctx, Dest, operandCreateLiteral(1));
        asmConditionalMove(ctx, Src, Dest, operandCreateLiteral(0));

    } else {
        char* DestStr = operandToStr(Dest);
        char* SrcStr = operandToStr(Src);

        if (operandGetSize(ctx->arch, Dest) > operandGetSize(ctx->arch, Src) &&
            Src.tag != operandLiteral)
            asmOutLn(ctx, "movzx %s, %s", DestStr, SrcStr);

        else
            asmOutLn(ctx, "mov %s, %s", DestStr, SrcStr);

        free(DestStr);
        free(SrcStr);
    }
}

void asmConditionalMove (struct asmCtx* ctx, operand Cond, operand Dest, operand Src) {
    operand FalseLabel = labelCreate(labelUndefined);
    asmBranch(ctx, operandCreateFlags(conditionNegate(Cond.condition)), FalseLabel);
    asmMove(ctx, Dest, Src);
    asmLabel(ctx, FalseLabel);
}

operand asmWiden (struct asmCtx* ctx, operand R, int size) {
    char* RStr = operandToStr(R);

    operand L;

    if (R.tag == operandReg) {
        R.base->allocatedAs = size;
        L = R;

    } else
        L = operandCreateReg(regAlloc(size));

    char* LStr = operandToStr(L);
    asmOutLn(ctx, "movsx %s, %s", LStr, RStr);
    free(LStr);
    free(RStr);

    if (R.tag != operandReg)
        operandFree(R);

    return L;
}

operand asmNarrow (struct asmCtx* ctx, operand R, int size) {
    operand L;

    if (R.tag == operandReg)
        L = R;

    else {
        L = operandCreateReg(regAlloc(operandGetSize(ctx->arch, R)));
        char* LStr = operandToStr(L);
        char* RStr = operandToStr(R);
        asmOutLn(ctx, "mov %s, %s", LStr, RStr);
        free(LStr);
        free(RStr);
        operandFree(R);
    }

    L.base->allocatedAs = size;
    return L;
}

void asmEvalAddress (asmCtx* ctx, operand L, operand R) {
    if (L.tag == operandMem && R.tag == operandMem) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx, intermediate, R);
        asmMove(ctx, L, intermediate);
        operandFree(intermediate);

    } else {
        char* LStr = operandToStr(L);
        R.size = ctx->arch->wordsize;
        char* RStr = operandToStr(R);
        asmOutLn(ctx, "lea %s, %s", LStr, RStr);
        free(LStr);
        free(RStr);
    }
}

void asmCompare (asmCtx* ctx, operand L, operand R) {
    if (   (L.tag == operandMem && R.tag == operandMem)
        || (L.tag == operandLiteral && R.tag == operandLiteral)) {
        operand intermediate = operandCreateReg(regAlloc(L.tag == operandMem ? max(L.size, R.size)
                                                                             : ctx->arch->wordsize));
        asmMove(ctx, intermediate, L);
        asmCompare(ctx, intermediate, R);
        operandFree(intermediate);

    } else if (L.tag == operandLiteral)
        asmCompare(ctx, R, L);

    else {
        char* LStr = operandToStr(L);
        char* RStr = operandToStr(R);
        asmOutLn(ctx, "cmp %s, %s", LStr, RStr);
        free(LStr);
        free(RStr);
    }
}

void asmBOP (asmCtx* ctx, boperation Op, operand L, operand R) {
    if (L.tag == operandMem && R.tag == operandMem) {
        operand intermediate = operandCreateReg(regAlloc(max(L.size, R.size)));
        asmMove(ctx, intermediate, R);
        asmBOP(ctx, Op, L, intermediate);
        operandFree(intermediate);

    /*imul mem, <...> isnt a thing
      Sucks, right?*/
    } else if (Op == bopMul && L.tag == operandMem) {
        if (R.tag == operandReg) {
            asmBOP(ctx, bopMul, R, L);
            asmMove(ctx, L, R);
            operandFree(R);

        } else {
            operand tmp = operandCreateReg(regAlloc(max(L.size, R.size)));

            char* LStr = operandToStr(L);
            char* RStr = operandToStr(R);
            char* tmpStr = operandToStr(tmp);
            asmOutLn(ctx, "imul %s, %s, %s", tmpStr, LStr, RStr);
            free(tmpStr);
            free(LStr);
            free(RStr);

            asmMove(ctx, L, tmp);
            operandFree(tmp);
        }

    } else {
        char* LStr = operandToStr(L);
        char* RStr = operandToStr(R);

        const char* OpStr = Op == bopAdd ? "add" :
                            Op == bopSub ? "sub" :
                            Op == bopMul ? "imul" :
                            Op == bopBitAnd ? "and" :
                            Op == bopBitOr ? "or" :
                            Op == bopBitXor ? "xor" :
                            Op == bopShR ? "sar" :
                            Op == bopShL ? "sal" : 0;

        if (OpStr)
            asmOutLn(ctx, "%s %s, %s", OpStr, LStr, RStr);

        else
            printf("asmBOP(): unhandled operator '%d'\n", Op);

        free(LStr);
        free(RStr);
    }
}

void asmUOP (asmCtx* ctx, uoperation Op, operand R) {
    char* RStr = operandToStr(R);

    if (Op == uopInc)
        asmOutLn(ctx, "add %s, 1", RStr);

    else if (Op == uopDec)
        asmOutLn(ctx, "sub %s, 1", RStr);

    else if (Op == uopNeg || Op == uopBitwiseNot)
        asmOutLn(ctx, "%s %s", Op == uopNeg ? "neg" : "not", RStr);

    else
        printf("asmUOP(): unhandled operator tag, %d", Op);

    free(RStr);
}
