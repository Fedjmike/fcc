#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "../inc/reg.h"
#include "../inc/emitter.h"
#include "../inc/emitter-value.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

static operand emitterBOP (emitterCtx* ctx, ast* Node);
static operand emitterAssignmentBOP (emitterCtx* ctx, ast* Node);
static operand emitterUOP (emitterCtx* ctx, ast* Node);
static operand emitterTOP (emitterCtx* ctx, ast* Node);
static operand emitterIndex (emitterCtx* ctx, ast* Node);
static operand emitterCall (emitterCtx* ctx, ast* Node);
static operand emitterSymbol (emitterCtx* ctx, ast* Node);
static operand emitterLiteral (emitterCtx* ctx, ast* Node);

operand emitterValue (emitterCtx* ctx, ast* Node, operand Dest) {
    operand Value;

    /*Calculate the value*/

    if (Node->class == astBOP)
        Value = emitterBOP(ctx, Node);

    else if (Node->class == astUOP)
        Value = emitterUOP(ctx, Node);

    else if (Node->class == astTOP)
        Value = emitterTOP(ctx, Node);

    else if (Node->class == astCall)
        Value = emitterCall(ctx, Node);

    else if (Node->class == astIndex)
        Value = emitterIndex(ctx, Node);

    else if (Node->class == astLiteral) {
        if (Node->litClass == literalIdent)
            Value = emitterSymbol(ctx, Node);

        else if (Node->litClass == literalInt || Node->litClass == literalBool)
            Value = emitterLiteral(ctx, Node);

        else
            debugErrorUnhandled("emitterValue", "literal class", literalClassGetStr(Node->litClass));

    } else
        debugErrorUnhandled("emitterValue", "AST class", astClassGetStr(Node->class));

    /*Put it where requested*/

    /*If they haven't specifically asked for the reference as memory
      then they're unaware it's held as a reference at all
      so make it a plain ol' value*/
    if (Value.class == operandMemRef && Dest.class != operandMem) {
        operand nValue = operandCreateReg(regAllocGeneral());
        asmEvalAddress(ctx->Asm, nValue, Value);
        operandFree(Value);
        Value = nValue;
    }

    if (Dest.class != operandUndefined) {
        if (Dest.class != Value.class) {
            if (Dest.class == operandFlags) {
                asmBOP(ctx->Asm, bopCmp, Value, operandCreateLiteral(0));
                operandFree(Value);

                Dest = operandCreateFlags(conditionEqual);

            } else if (Dest.class == operandReg) {
                /*If a specific register wasn't requested, allocate one*/
                if (Dest.reg == regUndefined)
                    Dest.reg = regAllocGeneral();

                asmMove(ctx->Asm, Dest, Value);
                operandFree(Value);

            } else if (Dest.class == operandMem) {
                if (Value.class == operandMemRef) {
                    Dest = Value;
                    Dest.class = operandMem;

                } else
                    printf("emitterValue(): unable to convert non lvalue operand class, %d.\n", Value.class);

            } else if (Dest.class == operandStack) {
                /*Larger than a word?*/
                if (Value.class == operandMem && Value.size > 8) {
                    int total = Value.size;

                    /*Then push on *backwards* in word chunks.
                      Start at the highest address*/
                    Value.offset += total-8;
                    Value.size = 8;

                    for (int i = 0; i < total; i += 8) {
                        asmPush(ctx->Asm, Value);
                        Value.offset -= 8;
                    }

                    Dest.size = total;

                } else {
                    asmPush(ctx->Asm, Value);
                    operandFree(Value);
                    Dest.size = 8;
                }

            } else if (Value.class == operandUndefined)
                printf("emitterValue(): expected value, void given.\n");

            else {
                debugErrorUnhandled("emitterValue", "operand class", operandClassGetStr(Value.class));
                debugErrorUnhandled("emitterValue", "operand class", operandClassGetStr(Dest.class));
            }

        } else if (Dest.class == operandReg && Dest.reg != Value.reg) {
            if (Dest.reg == regUndefined)
                Dest.reg = regAllocGeneral();

            asmMove(ctx->Asm, Dest, Value);
            operandFree(Value);

        } else
            Dest = Value;

    } else
        Dest = Value;

    return Dest;
}

operand emitterBOP (emitterCtx* ctx, ast* Node) {
    debugEnter("BOP");

    operand L;
    operand R;
    operand Value;

    if (!strcmp(Node->o, "=") ||
        !strcmp(Node->o, "+=") ||
        !strcmp(Node->o, "-=") ||
        !strcmp(Node->o, "*="))
        Value = emitterAssignmentBOP(ctx, Node);

    else if (!strcmp(Node->o, ".")) {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, operandCreate(operandMem));
        asmLeave(ctx->Asm);

        Value.offset += Node->r->symbol->offset;
        Value.size = typeGetSize(Node->r->symbol->dt);

    } else if (!strcmp(Node->o, "->")) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, operandCreateReg(regUndefined));
        asmLeave(ctx->Asm);

        Value = operandCreateMem(L.reg, Node->r->symbol->offset, typeGetSize(Node->r->dt));

    } else {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, operandCreateReg(regUndefined));
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        if (!strcmp(Node->o, "==") ||
            !strcmp(Node->o, "!=") ||
            !strcmp(Node->o, "<") ||
            !strcmp(Node->o, "<=") ||
            !strcmp(Node->o, ">" ) ||
            !strcmp(Node->o, ">=")) {
            Value = operandCreateFlags(conditionNegate(conditionFromStr(Node->o)));
            asmBOP(ctx->Asm, bopCmp, L, R);
            operandFree(L);

        } else {
            Value = L;

            if (!strcmp(Node->o, "+"))
                asmBOP(ctx->Asm, bopAdd, L, R);

            else if (!strcmp(Node->o, "-"))
                asmBOP(ctx->Asm, bopSub, L, R);

            else if (!strcmp(Node->o, "*"))
                asmBOP(ctx->Asm, bopMul, L, R);

            else
                debugErrorUnhandled("emitterBOP", "operator", Node->o);
        }

        operandFree(R);
    }

    debugLeave();

    return Value;
}

operand emitterAssignmentBOP (emitterCtx* ctx, ast* Node) {
    asmEnter(ctx->Asm);
    operand L = emitterValue(ctx, Node->l, operandCreate(operandMem));;
    operand R;
    asmLeave(ctx->Asm);

    operand Value;

    if (!strcmp(Node->o, "*=")) {
        /*imul m64, r/imm64 isnt a thing
          So we have to do:
          [request R -> reg]
          [request L -> mem]
          imul R:reg, L
          mov L, R:reg
          Sucks, right?*/

        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreate(operandReg));
        asmLeave(ctx->Asm);

        asmBOP(ctx->Asm, bopMul, R, L);
        asmMove(ctx->Asm, L, R);

    } else {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        if (!strcmp(Node->o, "="))
            asmMove(ctx->Asm, L, R);

        else if (!strcmp(Node->o, "+="))
            asmBOP(ctx->Asm, bopAdd, L, R);

        else if (!strcmp(Node->o, "-="))
            asmBOP(ctx->Asm, bopSub, L, R);

        else
            debugErrorUnhandled("emitterAssignmentBOP", "operator", Node->o);
    }

    Value = R;
    operandFree(L);

    return Value;
}

operand emitterUOP (emitterCtx* ctx, ast* Node) {
    debugEnter("UOP");

    operand Value;
    operand R;

    if (!strcmp(Node->o, "++") ||
        !strcmp(Node->o, "--")) {
        /*Specifically ask it to be stored in a register so that it's previous
          value is returned*/
        asmEnter(ctx->Asm);
        Value = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        if (!strcmp(Node->o, "++"))
            asmUOP(ctx->Asm, uopInc, R);

        else /*if (!strcmp(Node->o, "--"))*/
            asmUOP(ctx->Asm, uopDec, R);

    } else if (!strcmp(Node->o, "*")) {
        asmEnter(ctx->Asm);
        operand Ptr = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        asmLeave(ctx->Asm);

        R = operandCreateMem(Ptr.reg, 0, typeGetSize(Node->dt));
        Value = operandCreateReg(regAllocGeneral());

        asmMove(ctx->Asm, Value, R);
        operandFree(R);

    } else if (!strcmp(Node->o, "&")) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreate(operandMem));
        R.size = 8;
        asmLeave(ctx->Asm);

        Value = operandCreateReg(regAllocGeneral());

        asmEvalAddress(ctx->Asm, Value, R);

    } else
        debugErrorUnhandled("emitterUOP", "operator", Node->o);

    debugLeave();

    return Value;
}

operand emitterTOP (emitterCtx* ctx, ast* Node) {
    debugEnter("TOP");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);
    operand Value = operandCreateReg(regAllocGeneral());

    asmBranch(ctx->Asm,
              emitterValue(ctx,
                           Node->firstChild,
                           operandCreateFlags(conditionUndefined)),
              ElseLabel);

    /*Assert returns Value*/
    emitterValue(ctx, Node->l, Value);

    asmComment(ctx->Asm, "");
    asmJump(ctx->Asm, EndLabel);
    asmLabel(ctx->Asm, ElseLabel);

    emitterValue(ctx, Node->r, Value);

    asmLabel(ctx->Asm, EndLabel);

    debugLeave();

    return Value;
}

operand emitterIndex (emitterCtx* ctx, ast* Node) {
    debugEnter("Index");

    operand Value;

    operand L;
    operand R;

    /*Is it an array? Are we directly offsetting the address*/
    if (typeIsArray(Node->l->dt)) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, operandCreate(operandMem));
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        if (R.class == operandLiteral) {
            Value = L;
            Value.offset += typeGetSize(Node->l->dt->base) * R.literal;

        } else if (R.class == operandReg) {
            Value = L;
            Value.index = R.reg;
            Value.factor = typeGetSize(Node->l->dt->base);

        } else {
            operand index = operandCreateReg(regAllocGeneral());
            asmMove(ctx->Asm, index, R);
            operandFree(R);

            Value = L;
            Value.index = index.reg;
            Value.factor = typeGetSize(Node->l->dt->base);
        }

        Value.size = typeGetSize(Node->dt);

    /*Is it instead a pointer? Get value and offset*/
    } else /*if (typeIsPtr(Node->l->dt)*/ {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(typeGetSize(Node->l->dt->base)));
        asmLeave(ctx->Asm);

        L = emitterValue(ctx, Node->l, operandCreate(operandUndefined));

        asmBOP(ctx->Asm, bopAdd, R, L);
        operandFree(L);

        Value = operandCreateMem(R.reg, 0, typeGetSize(Node->dt));
    }

    debugLeave();

    return Value;
}

operand emitterCall (emitterCtx* ctx, ast* Node) {
    debugEnter("Call");

    operand Value;

    /*Save the general registers in use*/
    for (int reg = regRAX; reg <= regR15; reg++)
        if (regIsUsed(reg))
            asmPush(ctx->Asm, operandCreateReg(reg));

    int argSize = 0;

    /*Push the args on backwards (cdecl)*/
    for (ast* Current = Node->lastChild;
         Current;
         Current = Current->prevSibling) {
        operand Arg = emitterValue(ctx, Current, operandCreate(operandStack));
        argSize += Arg.size;
    }

    /*Get the address (usually, as a label) of the proc*/
    operand Proc = emitterValue(ctx, Node->l, operandCreate(operandUndefined));
    asmCall(ctx->Asm, Proc);
    operandFree(Proc);

    /*If RAX is already in use (currently backed up to the stack), relocate the
      return value to another free reg before RAXs value is restored.*/
    if (regRequest(regRAX))
        asmMove(ctx->Asm,
                operandCreateReg(regAllocGeneral()),
                operandCreateReg(regRAX));

    Value = operandCreateReg(regRAX);

    /*Pop the args*/
    asmPopN(ctx->Asm, argSize/8);

    /*Restore the saved registers (backwards as stacks are LIFO)*/
    for (regClass reg = regR15; reg >= regRAX; reg--)
        //Attempt to restore all but the one we just allocated for the ret value
        if (regIsUsed(reg) && reg != Value.reg)
            asmPop(ctx->Asm, operandCreateReg(reg));

    debugLeave();

    return Value;
}

operand emitterSymbol (emitterCtx* ctx, ast* Node) {
    debugEnter("Symbol");

    operand Value = operandCreate(operandUndefined);

    if (Node->symbol->class == symFunction)
        Value = Node->symbol->label;

    else if (Node->symbol->class == symVar || Node->symbol->class == symParam) {
        if (typeIsArray(Node->symbol->dt))
            Value = operandCreateMemRef(regRBP, Node->symbol->offset, typeGetSize(Node->symbol->dt->base));

        else
            Value = operandCreateMem(regRBP, Node->symbol->offset, typeGetSize(Node->symbol->dt));

    } else
        debugErrorUnhandled("emitterSymbol", "symbol class", symClassGetStr(Node->symbol->class));

    debugLeave();

    return Value;
}

operand emitterLiteral (emitterCtx* ctx, ast* Node) {
    debugEnter("Literal");

    operand Value;

    if (Node->litClass == literalInt)
        Value = operandCreateLiteral(*(int*) Node->literal);

    else if (Node->litClass == literalBool)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else
        debugErrorUnhandled("emitterLiteral", "literal class", literalClassGetStr(Node->litClass));

    debugLeave();

    return Value;
}
