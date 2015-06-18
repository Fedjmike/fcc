#include "../inc/asm-amd64.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/ir.h"
#include "../inc/asm.h"
#include "../inc/reg.h"

#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

void asmComment (asmCtx* ctx, const char* str) {
    asmOutLn(ctx, ";%s", str);
}

void asmFilePrologue (asmCtx* ctx) {
    asmOutLn(ctx, ".file 1 \"%s\"", ctx->filename);
    asmOutLn(ctx, ".intel_syntax noprefix");
}

void asmFileEpilogue (asmCtx* ctx) {
    (void) ctx;
}

void asmFnLinkageBegin (FILE* file, const char* name) {
    /*Symbol, linkage and alignment*/
    fprintf(file, ".balign 16\n");
    fprintf(file, ".globl %s\n", name);
    fprintf(file, "%s:\n", name);
}

void asmFnLinkageEnd (FILE* file, const char* name) {
    (void) file, (void) name;
}

void asmFnPrologue (irCtx* ir, irBlock* block, int localSize) {
    asmCtx* ctx = ir->asm;

    /*Register saving, create a new stack frame, stack variables etc*/

    asmPush(ir, block, ctx->basePtr);
    asmMove(ir, block, ctx->basePtr, ctx->stackPtr);

    if (localSize != 0)
        asmBOP(ir, block, bopSub, ctx->stackPtr, operandCreateLiteral(localSize));

    for (int i = 0; i < ctx->arch->calleeSaveRegs.length; i++) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->calleeSaveRegs, i);
        asmSaveReg(ir, block, r);
    }
}

void asmFnEpilogue (irCtx* ir, irBlock* block) {
    asmCtx* ctx = ir->asm;

    /*Pop off saved regs in reverse order*/
    for (int i = ctx->arch->calleeSaveRegs.length-1; i >= 0 ; i--) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->calleeSaveRegs, i);
        asmRestoreReg(ir, block, r);
    }

    asmMove(ir, block, ctx->stackPtr, ctx->basePtr);
    asmPop(ir, block, ctx->basePtr);
}

void asmSaveReg (irCtx* ir, irBlock* block, regIndex r) {
    asmCtx* ctx = ir->asm;
    irBlockOut(block, "push %s", regIndexGetName(r, ctx->arch->wordsize));
}

void asmRestoreReg (irCtx* ir, irBlock* block, regIndex r) {
    asmCtx* ctx = ir->asm;
    irBlockOut(block, "pop %s", regIndexGetName(r, ctx->arch->wordsize));
}

void asmDataSection (asmCtx* ctx) {
    asmOutLn(ctx, ".section .data");
}

void asmRODataSection (asmCtx* ctx) {
    asmOutLn(ctx, ".section .rodata");
}

void asmStaticData (asmCtx* ctx, const char* label, bool global, int size, intptr_t initial) {
    if (global)
        asmOutLn(ctx, ".globl %s", label);

    asmOutLn(ctx, "%s:", label);

    if (size == 1)
        asmOutLn(ctx, ".byte %d", initial);

    else if (size == 2)
        asmOutLn(ctx, ".word %d", initial);

    else if (size == 4)
        asmOutLn(ctx, ".quad %d", initial);

    else if (size == 8)
        asmOutLn(ctx, ".octa %d", initial);

    else
        debugErrorUnhandledInt("asmStaticData", "data size", size);
}

void asmStringConstant (asmCtx* ctx, const char* label, const char* str) {
    asmOutLn(ctx, "%s:", label);
    asmOutLn(ctx, ".asciz \"%s\"", str);
}

void asmLabel (asmCtx* ctx, const char* label) {
    asmOutLn(ctx, "\t%s:", label);
}

void asmJump (asmCtx* ctx, const char* label) {
    asmOutLn(ctx, "jmp %s", label);
}

void asmBranch (asmCtx* ctx, operand Condition, const char* label) {
    char* CStr = operandToStr(Condition);
    asmOutLn(ctx, "j%s %s", CStr, label);
    free(CStr);
}

void asmCall (asmCtx* ctx, const char* label) {
    asmOutLn(ctx, "call %s", label);
}

void asmCallIndirect (irBlock* block, operand L) {
    char* LStr = operandToStr(L);
    irBlockOut(block, "call %s", LStr);
    free(LStr);
}

void asmReturn (asmCtx* ctx) {
    asmOutLn(ctx, "ret");
}

void asmPush (irCtx* ir, irBlock* block, operand L) {
    asmCtx* ctx = ir->asm;

    /*Flags*/
    if (L.tag == operandFlags) {
        asmPush(ir, block, operandCreateLiteral(0));
        operand top = operandCreateMem(ctx->stackPtr.base, 0, ctx->arch->wordsize);
        asmConditionalMove(ir, block, L, top, operandCreateLiteral(1));

    /*Larger than word*/
    } else if (operandGetSize(ctx->arch, L) > ctx->arch->wordsize) {
        if (debugAssert("asmPush", "memory operand", L.tag == operandMem))
            return;

        int size = operandGetSize(ctx->arch, L);

        /*Push on *backwards* in word chunks.
          Start at the highest address*/
        L.offset += size;
        L.size = ctx->arch->wordsize;

        for (int i = 0; i < size; i += ctx->arch->wordsize) {
            L.offset -= ctx->arch->wordsize;
            asmPush(ir, block, L);
        }

    /*Smaller than a word*/
    } else if (L.tag == operandMem && operandGetSize(ctx->arch, L) < ctx->arch->wordsize) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmMove(ir, block, intermediate, L);
        asmPush(ir, block, intermediate);
        operandFree(intermediate);

    } else {
        char* LStr = operandToStr(L);
        irBlockOut(block, "push %s", LStr);
        free(LStr);
    }
}

void asmPop (irCtx* ir, irBlock* block, operand L) {
    (void) ir;

    char* LStr = operandToStr(L);
    irBlockOut(block, "pop %s", LStr);
    free(LStr);
}

void asmPushN (irCtx* ir, irBlock* block, int n) {
    asmCtx* ctx = ir->asm;

    if (n)
        asmBOP(ir, block, bopSub, ctx->stackPtr, operandCreateLiteral(n*ctx->arch->wordsize));
}

void asmPopN (irCtx* ir, irBlock* block, int n) {
    asmCtx* ctx = ir->asm;

    if (n)
        asmBOP(ir, block, bopAdd, ctx->stackPtr, operandCreateLiteral(n*ctx->arch->wordsize));
}

static bool operandIsMem (operand L) {
    return L.tag == operandMem || L.tag == operandLabelMem;
}

void asmMove (irCtx* ir, irBlock* block, operand Dest, operand Src) {
    asmCtx* ctx = ir->asm;

    if (Dest.tag == operandInvalid || Src.tag == operandInvalid)
        return;

    /*Too big for single register*/
    else if (operandGetSize(ctx->arch, Dest) > ctx->arch->wordsize) {
        if (   debugAssert("asmMove", "Dest mem", Dest.tag == operandMem)
            || debugAssert("asmMove", "Src mem", Src.tag == operandMem)
            || debugAssert("asmMove", "operand size equality",
                           operandGetSize(ctx->arch, Dest) == operandGetSize(ctx->arch, Src)))
            return;

        int size = operandGetSize(ctx->arch, Dest);
        int chunk = ctx->arch->wordsize;
        Dest.size = Src.size = chunk;

        /*Move up the operands, so far as a chunk would not go past their ends*/
        for (int i = 0;
             i+chunk <= size;
             i += chunk, Dest.offset += chunk, Src.offset += chunk)
            asmMove(ir, block, Dest, Src);

        /*Were the operands not an even multiple of the chunk size?*/
        if (size % chunk != 0) {
            /*Final chunk size is the remainder*/
            Dest.size = Src.size = size % chunk;
            asmMove(ir, block, Dest, Src);
        }

    /*Both memory operands*/
    } else if (operandIsMem(Dest) && operandIsMem(Src)) {
        operand intermediate = operandCreateReg(regAlloc(max(Dest.size, Src.size)));
        asmMove(ir, block, intermediate, Src);
        asmMove(ir, block, Dest, intermediate);
        operandFree(intermediate);

    /*Flags*/
    } else if (Src.tag == operandFlags) {
        asmMove(ir, block, Dest, operandCreateLiteral(0));
        asmConditionalMove(ir, block, Src, Dest, operandCreateLiteral(1));

    } else {
        char* DestStr = operandToStr(Dest);
        char* SrcStr = operandToStr(Src);

        if (   operandGetSize(ctx->arch, Dest) > operandGetSize(ctx->arch, Src)
            && Src.tag != operandLiteral)
            irBlockOut(block, "movzx %s, %s", DestStr, SrcStr);

        else
            irBlockOut(block, "mov %s, %s", DestStr, SrcStr);

        free(DestStr);
        free(SrcStr);
    }
}

void asmConditionalMove (irCtx* ir, irBlock* block, operand Cond, operand Dest, operand Src) {
    char falseLabel[10];
    sprintf(falseLabel, ".%X", ir->labelNo++);

    Cond.condition = conditionNegate(Cond.condition);
    char* cond = operandToStr(Cond);

    irBlockOut(block, "j%s %s", cond, falseLabel);
    asmMove(ir, block, Dest, Src);
    irBlockOut(block, "%s:", falseLabel);

    free(cond);
}

void asmRepStos (irCtx* ir, irBlock* block, operand RAX, operand RCX, operand RDI,
                 operand Dest, int length, operand Src) {
    int chunksize = ir->arch->wordsize,
        iterations = length/chunksize;

    asmMove(ir, block, RAX, Src);
    asmMove(ir, block, RCX, operandCreateLiteral(iterations));
    asmEvalAddress(ir, block, RDI, Dest);

    irBlockOut(block, "rep stos%s", chunksize == 8 ? "q" : "d");
}

void asmEvalAddress (irCtx* ir, irBlock* block, operand L, operand R) {
    asmCtx* ctx = ir->asm;

    if (operandIsMem(L) && operandIsMem(R)) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ir, block, intermediate, R);
        asmMove(ir, block, L, intermediate);
        operandFree(intermediate);

    } else {
        char* LStr = operandToStr(L);
        R.size = ctx->arch->wordsize;
        char* RStr = operandToStr(R);
        irBlockOut(block, "lea %s, %s", LStr, RStr);
        free(LStr);
        free(RStr);
    }
}

void asmCompare (irCtx* ir, irBlock* block, operand L, operand R) {
    asmCtx* ctx = ir->asm;

    if (   (operandIsMem(L) && operandIsMem(R))
        || (L.tag == operandLiteral && R.tag == operandLiteral)) {
        operand intermediate = operandCreateReg(regAlloc(L.tag == operandMem ? max(L.size, R.size)
                                                                             : ctx->arch->wordsize));
        asmMove(ir, block, intermediate, L);
        asmCompare(ir, block, intermediate, R);
        operandFree(intermediate);

    } else if (L.tag == operandLiteral) {
        asmCompare(ir, block, R, L);

    } else {
        char* LStr = operandToStr(L);
        char* RStr = operandToStr(R);
        irBlockOut(block, "cmp %s, %s", LStr, RStr);
        free(LStr);
        free(RStr);
    }
}

void asmBOP (irCtx* ir, irBlock* block, boperation Op, operand L, operand R) {
    if (operandIsMem(L) && operandIsMem(R)) {
        operand intermediate = operandCreateReg(regAlloc(max(L.size, R.size)));
        asmMove(ir, block, intermediate, R);
        asmBOP(ir, block, Op, L, intermediate);
        operandFree(intermediate);

    /*imul mem, <...> isnt a thing
      Sucks, right?*/
    } else if (Op == bopMul && L.tag == operandMem) {
        if (R.tag == operandReg) {
            asmBOP(ir, block, bopMul, R, L);
            asmMove(ir, block, L, R);
            operandFree(R);

        } else {
            operand tmp = operandCreateReg(regAlloc(max(L.size, R.size)));

            char* LStr = operandToStr(L);
            char* RStr = operandToStr(R);
            char* tmpStr = operandToStr(tmp);
            irBlockOut(block, "imul %s, %s, %s", tmpStr, LStr, RStr);
            free(tmpStr);
            free(LStr);
            free(RStr);

            asmMove(ir, block, L, tmp);
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
            irBlockOut(block, "%s %s, %s", OpStr, LStr, RStr);

        else
            printf("asmBOP(): unhandled operator '%d'\n", Op);

        free(LStr);
        free(RStr);
    }
}

void asmDivision (irCtx* ir, irBlock* block, operand R) {
    (void) ir;

    char* RStr = operandToStr(R);
    irBlockOut(block, "idiv %s", RStr);
    free(RStr);
}

void asmUOP (irCtx* ir, irBlock* block, uoperation Op, operand R) {
    (void) ir;

    char* RStr = operandToStr(R);

    if (Op == uopInc)
        irBlockOut(block, "add %s, 1", RStr);

    else if (Op == uopDec)
        irBlockOut(block, "sub %s, 1", RStr);

    else if (Op == uopNeg || Op == uopBitwiseNot)
        irBlockOut(block, "%s %s", Op == uopNeg ? "neg" : "not", RStr);

    else
        printf("asmUOP(): unhandled operator tag, %d", Op);

    free(RStr);
}
