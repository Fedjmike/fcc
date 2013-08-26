#include "../inc/asm-amd64.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/asm.h"
#include "../inc/reg.h"

#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

void asmComment (asmCtx* ctx, char* format, ...) {
    asmOutLn(ctx, ";");

    va_list args;
    va_start(args, format);
    asmVarOut(ctx, format, args);
    va_end(args);
}

void asmFilePrologue (asmCtx* ctx) {
    asmOutLn(ctx, ".intel_syntax noprefix");
}

void asmFileEpilogue (asmCtx* ctx) {
    (void) ctx;
}

void asmFnPrologue (asmCtx* ctx, operand Name, int localSize) {
    /*Symbol, linkage and alignment*/
    asmOutLn(ctx, ".balign 16");
    asmOutLn(ctx, ".globl %s", labelGet(Name));
    asmOutLn(ctx, "%s:", labelGet(Name));

    /*Register saving, create a new stack frame, stack variables etc*/

    asmPush(ctx, ctx->basePtr);
    asmMove(ctx, ctx->basePtr, ctx->stackPtr);

    if (localSize != 0)
        asmBOP(ctx, bopSub, ctx->stackPtr, operandCreateLiteral(localSize));
}

void asmFnEpilogue (asmCtx* ctx, operand EndLabel) {
    /*Exit stack frame*/
    asmOutLn(ctx, "%s:", labelGet(EndLabel));
    asmMove(ctx, ctx->stackPtr, ctx->basePtr);
    asmPop(ctx, ctx->basePtr);
    asmOutLn(ctx, "ret");
}

void asmStringConstant (struct asmCtx* ctx, operand label, char* str) {
    asmOutLn(ctx, ".section .rodata");
    asmOutLn(ctx, "%s:", labelGet(label));
    asmOutLn(ctx, ".ascii \"%s\\0\"", str);    /* .ascii "%s\0" */
    asmOutLn(ctx, ".section .text");
}

void asmLabel (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "%s:", LStr);
    free(LStr);
}

void asmJump (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "jmp %s", LStr);
    free(LStr);
}

void asmBranch (asmCtx* ctx, operand Condition, operand L) {
    char* CStr = operandToStr(Condition);
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "j%s %s", CStr, LStr);
    free(CStr);
    free(LStr);
}

void asmPush (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "push %s", LStr);
    free(LStr);
}

void asmPop (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "pop %s", LStr);
    free(LStr);
}

void asmPopN (asmCtx* ctx, int n) {
    asmBOP(ctx, bopAdd, ctx->stackPtr, operandCreateLiteral(n*ctx->arch->wordsize));
}

void asmMove (asmCtx* ctx, operand Dest, operand Src) {
    if (Dest.tag == operandMem && Src.tag == operandMem) {
        operand intermediate = operandCreateReg(regAlloc(Dest.size > Src.size ? Dest.size : Src.size));
        asmMove(ctx, intermediate, Src);
        asmMove(ctx, Dest, intermediate);
        operandFree(intermediate);

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

void asmEvalAddress (asmCtx* ctx, operand L, operand R) {
    if (L.tag == operandMem && R.tag == operandMem) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx, intermediate, R);
        asmMove(ctx, L, intermediate);
        operandFree(intermediate);

    } else {
        char* LStr = operandToStr(L);
        char* RStr = operandToStr(R);
        asmOutLn(ctx, "lea %s, %s", LStr, RStr);
        free(LStr);
        free(RStr);
    }
}

void asmBOP (asmCtx* ctx, boperation Op, operand L, operand R) {
    if (L.tag == operandMem && R.tag == operandMem) {
        /*Unlike lea, perform the op after the move. This is because these
          ops affect the flags (particularly cmp)*/
        operand intermediate = operandCreateReg(regAlloc(L.size > R.size ? L.size : R.size));
        asmMove(ctx, intermediate, R);
        asmBOP(ctx, Op, L, intermediate);
        operandFree(intermediate);

    } else {
        char* LStr = operandToStr(L);
        char* RStr = operandToStr(R);

        const char* OpStr = Op == bopCmp ? "cmp" :
                            Op == bopAdd ? "add" :
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

void asmCall (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "call %s", LStr);
    free(LStr);
}
