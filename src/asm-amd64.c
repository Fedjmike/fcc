#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

void asmComment (char* format, ...) {
    asmOut(";");

    va_list args;
    va_start(args, format);
    asmVarOut(format, args);
    va_end(args);
}

void asmFilePrologue () {
    asmOut(".intel_syntax noprefix");
}

void asmFileEpilogue () {
}

void asmFnPrologue (char* Name, int LocalSize) {
    /*Symbol, linkage and alignment*/
    asmOut(".balign 16");
    asmOut(".globl %s", Name);
    asmOut("%s:", Name);

    /*Register saving, create a new stack frame, stack variables etc*/

    asmOut("push rbp");
    asmOut("mov rbp, rsp");

    if (LocalSize != 0)
        asmOut("sub rsp, %d", LocalSize);
}

void asmFnEpilogue (char* EndLabel) {
    /*Exit stack frame*/
    asmOut("%s:", EndLabel);
    asmOut("mov rsp, rbp");
    asmOut("pop rbp");
    asmOut("ret");
}

void asmLabel (operand L) {
    char* LStr = operandToStr(L);
    asmOut("%s:", LStr);
    free(LStr);
}

void asmJump (operand L) {
    char* LStr = operandToStr(L);
    asmOut("jmp %s", LStr);
    free(LStr);
}

void asmBranch (operand Condition, operand L) {
    char* CStr = operandToStr(Condition);
    char* LStr = operandToStr(L);
    asmOut("j%s %s", CStr, LStr);
    free(CStr);
    free(LStr);
}

void asmPush (operand L) {
    char* LStr = operandToStr(L);
    asmOut("push %s", LStr);
    free(LStr);
}

void asmPop (operand L) {
    char* LStr = operandToStr(L);
    asmOut("pop %s", LStr);
    free(LStr);
}

void asmPopN (int n) {
    asmOut("add rsp, %d", n*8);
}

void asmMove (operand L, operand R) {
    reportRegs();

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);

    if (operandGetSize(L) > operandGetSize(R) &&
        R.class != operandLiteral)
        asmOut("movzx %s, %s", LStr, RStr);

    else
        asmOut("mov %s, %s", LStr, RStr);

    free(LStr);
    free(RStr);
}

void asmEvalAddress (operand L, operand R) {
    reportRegs();

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);
    asmOut("lea %s, %s", LStr, RStr);
    free(LStr);
    free(RStr);
}

void asmBOP (boperation Op, operand L, operand R) {
    reportRegs();

    char* LStr = operandToStr(L);
    char* RStr = operandToStr(R);

    if (Op == bopCmp)
        asmOut("cmp %s, %s", LStr, RStr);

    else if (Op == bopMul)
        asmOut("imul %s, %s", LStr, RStr);

    else if (Op == bopAdd)
        asmOut("add %s, %s", LStr, RStr);

    else if (Op == bopSub)
        asmOut("sub %s, %s", LStr, RStr);

    else
        printf("asmBOP(): unhandled operator '%d'\n", Op);

    free(LStr);
    free(RStr);
}

void asmUOP (uoperation Op, operand L) {
    char* LStr = operandToStr(L);

    if (Op == uopInc)
        asmOut("add %s, 1", LStr);

    else if (Op == uopDec)
        asmOut("sub %s, 1", LStr);

    else
        printf("asmUOP(): unhandled operator class, %d", Op);

    free(LStr);
}

void asmCall (operand L) {
    char* LStr = operandToStr(L);
    asmOut("call %s", LStr);
    free(LStr);
}
