#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

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
}

void asmFnPrologue (asmCtx* ctx, char* Name, int LocalSize) {
    /*Symbol, linkage and alignment*/
    asmOutLn(ctx, ".balign 16");
    asmOutLn(ctx, ".globl %s", Name);
    asmOutLn(ctx, "%s:", Name);

    /*Register saving, create a new stack frame, stack variables etc*/

    asmOutLn(ctx, "push rbp");
    asmOutLn(ctx, "mov rbp, rsp");

    if (LocalSize != 0)
        asmOutLn(ctx, "sub rsp, %d", LocalSize);
}

void asmFnEpilogue (asmCtx* ctx, char* EndLabel) {
    /*Exit stack frame*/
    asmOutLn(ctx, "%s:", EndLabel);
    asmOutLn(ctx, "mov rsp, rbp");
    asmOutLn(ctx, "pop rbp");
    asmOutLn(ctx, "ret");
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
    asmOutLn(ctx, "add rsp, %d", n*8);
}

void asmMove (asmCtx* ctx, operand L, operand R) {
    reportRegs();

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);

    if (operandGetSize(L) > operandGetSize(R) &&
        R.class != operandLiteral)
        asmOutLn(ctx, "movzx %s, %s", LStr, RStr);

    else
        asmOutLn(ctx, "mov %s, %s", LStr, RStr);

    free(LStr);
    free(RStr);
}

void asmEvalAddress (asmCtx* ctx, operand L, operand R) {
    reportRegs();

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);
    asmOutLn(ctx, "lea %s, %s", LStr, RStr);
    free(LStr);
    free(RStr);
}

void asmBOP (asmCtx* ctx, boperation Op, operand L, operand R) {
    reportRegs();

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);

    if (Op == bopCmp)
        asmOutLn(ctx, "cmp %s, %s", LStr, RStr);

    else if (Op == bopMul)
        asmOutLn(ctx, "imul %s, %s", LStr, RStr);

    else if (Op == bopAdd)
        asmOutLn(ctx, "add %s, %s", LStr, RStr);

    else if (Op == bopSub)
        asmOutLn(ctx, "sub %s, %s", LStr, RStr);

    else
        printf("asmBOP(): unhandled operator '%d'\n", Op);

    free(LStr);
    free(RStr);
}

void asmUOP (asmCtx* ctx, uoperation Op, operand L) {
    char* LStr = operandToStr(L);

    if (Op == uopInc)
        asmOutLn(ctx, "add %s, 1", LStr);

    else if (Op == uopDec)
        asmOutLn(ctx, "sub %s, 1", LStr);

    else
        printf("asmUOP(): unhandled operator class, %d", Op);

    free(LStr);
}

void asmCall (asmCtx* ctx, operand L) {
    char* LStr = operandToStr(L);
    asmOutLn(ctx, "call %s", LStr);
    free(LStr);
}
