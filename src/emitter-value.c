#include "string.h"
#include "stdio.h"
#include "stdlib.h"

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

static operand emitterBOP (ast* Node);
static operand emitterAssign (ast* Node);
static operand emitterUOP (ast* Node);
static operand emitterTOP (ast* Node);
static operand emitterIndex (ast* Node);
static operand emitterCall (ast* Node);
static operand emitterSymbol (ast* Node);
static operand emitterLiteral (ast* Node);

operand emitterValue (ast* Node, operand Dest) {
    operand Value;

    /*Calculate the value*/

    if (Node->class == astBOP)
        Value = emitterBOP(Node);

    else if (Node->class == astUOP)
        Value = emitterUOP(Node);

    else if (Node->class == astTOP)
        Value = emitterTOP(Node);

    else if (Node->class == astCall)
        Value = emitterCall(Node);

    else if (Node->class == astIndex)
        Value = emitterIndex(Node);

    else if (Node->class == astLiteral) {
        if (Node->litClass == literalIdent)
            Value = emitterSymbol(Node);

        else if (Node->litClass == literalInt || Node->litClass == literalBool)
            Value = emitterLiteral(Node);

        else
            printf("emitterValue: unhandled literal class, %d.\n", Node->litClass);

    } else
        printf("emitterValue(): unhandled AST class, %d.\n", Node->class);

    /*Put it where requested*/

    /*If they haven't specifically asked for the reference as memory
      then they're unaware it's held as a reference at all
      so make it a plain ol' value*/
    if (Value.class == operandMemRef && Dest.class != operandMem) {
        operand nValue = operandCreateReg(regAllocGeneral());
        asmEvalAddress(nValue, Value);
        operandFree(Value);
        Value = nValue;
    }

    if (Dest.class != operandUndefined) {
        if (Dest.class != Value.class) {
            if (Dest.class == operandFlags) {
                asmBOP(bopCmp, Value, operandCreateLiteral(0));
                operandFree(Value);

                Dest = operandCreateFlags(conditionEqual);

            } else if (Dest.class == operandReg) {
                /*If a specific register wasn't requested, allocate one*/
                if (Dest.reg == regUndefined)
                    Dest.reg = regAllocGeneral();

                asmMove(Dest, Value);
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
                        asmPush(Value);
                        Value.offset -= 8;
                    }

                    Dest.size = total;

                } else {
                    asmPush(Value);
                    operandFree(Value);
                    Dest.size = 8;
                }

            } else if (Value.class == operandUndefined)
                printf("emitterValue(): expected value, void given.\n");

            else
                printf("emitterValue(): unhandled operand class, %d, %d.\n", Dest.class, Value.class);

        } else if (Dest.class == operandReg && Dest.reg != Value.reg) {
            if (Dest.reg == regUndefined)
                Dest.reg = regAllocGeneral();

            asmMove(Dest, Value);
            operandFree(Value);

        } else
            Dest = Value;

    } else
        Dest = Value;

    return Dest;
}

operand emitterBOP (ast* Node) {
    puts("BOP+");

    operand L;
    operand R;
    operand Value;

    if (!strcmp(Node->o, "=") ||
        !strcmp(Node->o, "+=") ||
        !strcmp(Node->o, "-=") ||
        !strcmp(Node->o, "*="))
        Value = emitterAssign(Node);

    else if (!strcmp(Node->o, ".")) {
        asmEnter();
        Value = L = emitterValue(Node->l, operandCreate(operandMem));
        asmLeave();

        Value.offset += Node->r->symbol->offset;
        Value.size = typeGetSize(Node->r->symbol->dt);

    } else if (!strcmp(Node->o, "->")) {
        asmEnter();
        L = emitterValue(Node->l, operandCreateReg(regUndefined));
        asmLeave();

        Value = operandCreateMem(L.reg, Node->r->symbol->offset, typeGetSize(Node->r->dt));

    } else {
        asmEnter();
        L = emitterValue(Node->l, operandCreateReg(regUndefined));
        R = emitterValue(Node->r, operandCreate(operandUndefined));
        asmLeave();

        if (!strcmp(Node->o, "==") ||
            !strcmp(Node->o, "!=") ||
            !strcmp(Node->o, "<") ||
            !strcmp(Node->o, "<=") ||
            !strcmp(Node->o, ">" ) ||
            !strcmp(Node->o, ">=")) {
            Value = operandCreateFlags(conditionNegate(conditionFromStr(Node->o)));
            asmBOP(bopCmp, L, R);
            operandFree(L);

        } else {
            Value = L;

            if (!strcmp(Node->o, "+"))
                asmBOP(bopAdd, L, R);

            else if (!strcmp(Node->o, "-"))
                asmBOP(bopSub, L, R);

            else if (!strcmp(Node->o, "*"))
                asmBOP(bopMul, L, R);

            else
                printf("emitterBOP(): unhandled operator '%s'\n", Node->o);
        }

        operandFree(R);
    }

    puts("-");

    return Value;
}

operand emitterAssign (ast* Node) {
    asmEnter();
    operand L = emitterValue(Node->l, operandCreate(operandMem));;
    operand R;
    asmLeave();

    operand Value;

    if (!strcmp(Node->o, "*=")) {
        /*imul m64, r/imm64 isnt a thing
          So we have to do:
          [request R -> reg]
          [request L -> mem]
          imul R:reg, L
          mov L, R:reg
          Sucks, right?*/

        asmEnter();
        R = emitterValue(Node->r, operandCreate(operandReg));
        asmLeave();

        asmBOP(bopMul, R, L);
        asmMove(L, R);

    } else {
        asmEnter();
        R = emitterValue(Node->r, operandCreate(operandUndefined));
        asmLeave();

        if (!strcmp(Node->o, "="))
            asmMove(L, R);

        else if (!strcmp(Node->o, "+="))
            asmBOP(bopAdd, L, R);

        else if (!strcmp(Node->o, "-="))
            asmBOP(bopSub, L, R);

        else
            printf("emitterBOP(): unhandled operator '%s'\n", Node->o);
    }

    Value = R;
    operandFree(L);

    return Value;
}

operand emitterUOP (ast* Node) {
    puts("UOP+");

    operand Value;
    operand R;

    if (!strcmp(Node->o, "++") ||
        !strcmp(Node->o, "--")) {
        /*Specifically ask it to be stored in a register so that it's previous
          value is returned*/
        asmEnter();
        Value = emitterValue(Node->r, operandCreateReg(regUndefined));
        R = emitterValue(Node->r, operandCreate(operandUndefined));
        asmLeave();

        if (!strcmp(Node->o, "++"))
            asmUOP(uopInc, R);

        else /*if (!strcmp(Node->o, "--"))*/
            asmUOP(uopDec, R);

    } else if (!strcmp(Node->o, "*")) {
        asmEnter();
        operand Ptr = emitterValue(Node->r, operandCreateReg(regUndefined));
        asmLeave();

        R = operandCreateMem(Ptr.reg, 0, typeGetSize(Node->dt));
        Value = operandCreateReg(regAllocGeneral());

        asmMove(Value, R);
        operandFree(R);

    } else if (!strcmp(Node->o, "&")) {
        asmEnter();
        R = emitterValue(Node->r, operandCreate(operandMem));
        R.size = 8;
        asmLeave();

        Value = operandCreateReg(regAllocGeneral());

        asmEvalAddress(Value, R);

    } else
        printf("emitterUOP(): unhandled operator '%s'\n", Node->o);

    puts("-");

    return Value;
}

operand emitterTOP (ast* Node) {
    puts("TOP+");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);
    operand Value = operandCreateReg(regAllocGeneral());

    asmBranch(emitterValue(Node->firstChild, operandCreateFlags(conditionUndefined)),
              ElseLabel);

    /*Assert returns Value*/
    emitterValue(Node->l, Value);

    asmComment("");
    asmJump(EndLabel);
    asmLabel(ElseLabel);

    emitterValue(Node->r, Value);

    asmLabel(EndLabel);

    puts("-");

    return Value;
}

operand emitterIndex (ast* Node) {
    puts("Index+");

    operand Value;

    operand L;
    operand R;

    /*Is it an array? Are we directly offsetting the address*/
    if (Node->l->dt.array != 0) {
        asmEnter();
        L = emitterValue(Node->l, operandCreate(operandMem));
        R = emitterValue(Node->r, operandCreate(operandUndefined));
        asmLeave();

        if (R.class == operandLiteral) {
            Value = L;
            Value.offset += Node->l->dt.basic->size*R.literal;

        } else if (R.class == operandReg) {
            Value = L;
            Value.index = R.reg;
            Value.factor = Node->l->dt.basic->size;

        } else {
            operand index = operandCreateReg(regAllocGeneral());
            asmMove(index, R);
            operandFree(R);

            Value = L;
            Value.index = index.reg;
            Value.factor = Node->l->dt.basic->size;
        }

        Value.size = typeGetSize(Node->dt);

    /*Is it instead a pointer? Get value and offset*/
    } else {
        asmEnter();
        R = emitterValue(Node->r, operandCreateReg(regUndefined));
        asmBOP(bopMul, R, operandCreateLiteral(Node->l->dt.basic->size));
        asmLeave();

        L = emitterValue(Node->l, operandCreate(operandUndefined));

        asmBOP(bopAdd, R, L);
        operandFree(L);

        Value = operandCreateMem(R.reg, 0, typeGetSize(Node->dt));
    }

    reportType(Node->dt);

    puts("-");

    return Value;
}

operand emitterCall (ast* Node) {
    puts("Call+");

    operand Value;

    /*Save the general registers in use*/
    for (int reg = regRAX; reg <= regR15; reg++)
        if (regIsUsed(reg))
            asmPush(operandCreateReg(reg));

    int argSize = 0;

    /*Push the args on backwards (cdecl)*/
    for (ast* Current = Node->lastChild;
         Current;
         Current = Current->prevSibling) {
        operand Arg = emitterValue(Current, operandCreate(operandStack));
        argSize += Arg.size;
    }

    /*Get the address (usually, as a label) of the proc*/
    operand Proc = emitterValue(Node->l, operandCreate(operandUndefined));
    asmCall(Proc);
    operandFree(Proc);

    /*If RAX is already in use (currently backed up to the stack), relocate the
      return value to another free reg before RAXs value is restored.*/
    if (regRequest(regRAX))
        asmMove(operandCreateReg(regAllocGeneral()),
                 operandCreateReg(regRAX));

    Value = operandCreateReg(regRAX);

    /*Pop the args*/
    asmPopN(argSize/8);

    /*Restore the saved registers (backwards as stacks are LIFO)*/
    for (int reg = regR15; reg >= regRAX; reg--)
        //Attempt to restore all but the one we just allocated for the ret value
        if (regIsUsed(reg) && reg != Value.reg)
            asmPop(operandCreateReg(reg));

    puts("-");

    return Value;
}

operand emitterSymbol (ast* Node) {
    puts("Symbol+");

    operand Value = operandCreate(operandUndefined);

    if (Node->symbol->class == symFunction)
        Value = Node->symbol->label;

    else if (Node->symbol->class == symVar || Node->symbol->class == symPara) {
        if (Node->symbol->dt.array == 0)
            Value = operandCreateMem(regRBP, Node->symbol->offset, Node->symbol->dt.basic->size);

        else
            Value = operandCreateMemRef(regRBP, Node->symbol->offset, Node->symbol->dt.basic->size);

    } else
        printf("emitterSymbol(): unhandled symbol class, %d\n", Node->symbol->class);

    puts("-");

    return Value;
}

operand emitterLiteral (ast* Node) {
    puts("Literal+");

    operand Value;

    if (Node->litClass == literalInt)
        Value = operandCreateLiteral(*(int*) Node->literal);

    else if (Node->litClass == literalBool)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else
        printf("emitterLiteral(): unhandled literal class, %d.\n", Node->litClass);

    puts("-");

    return Value;
}
