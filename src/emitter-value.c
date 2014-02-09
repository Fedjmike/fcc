#include "../inc/emitter-value.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/architecture.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"
#include "../inc/reg.h"

#include "../inc/emitter.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

static operand emitterBOP (emitterCtx* ctx, const ast* Node);
static operand emitterAssignmentBOP (emitterCtx* ctx, const ast* Node);
static operand emitterLogicalBOP (emitterCtx* ctx, const ast* Node);

static operand emitterUOP (emitterCtx* ctx, const ast* Node);
static operand emitterTOP (emitterCtx* ctx, const ast* Node);
static operand emitterIndex (emitterCtx* ctx, const ast* Node);
static operand emitterCall (emitterCtx* ctx, const ast* Node);
static operand emitterSizeof (emitterCtx* ctx, const ast* Node);
static operand emitterSymbol (emitterCtx* ctx, const ast* Node);
static operand emitterLiteral (emitterCtx* ctx, const ast* Node);
static operand emitterCompoundLiteral (emitterCtx* ctx, const ast* Node);

operand emitterValue (emitterCtx* ctx, const ast* Node, operand Dest) {
    operand Value;

    /*Calculate the value*/

    if (Node->tag == astBOP) {
        if (   !strcmp(Node->o, "=")
            || !strcmp(Node->o, "+=") || !strcmp(Node->o, "-=")
            || !strcmp(Node->o, "*=")
            || !strcmp(Node->o, "&=") || !strcmp(Node->o, "|=")
            || !strcmp(Node->o, "^=")
            || !strcmp(Node->o, ">>=") || !strcmp(Node->o, "<<="))
            Value = emitterAssignmentBOP(ctx, Node);

        else if (!strcmp(Node->o, "&&") || !strcmp(Node->o, "||"))
            Value = emitterLogicalBOP(ctx, Node);

        else
            Value = emitterBOP(ctx, Node);

    } else if (Node->tag == astUOP)
        Value = emitterUOP(ctx, Node);

    else if (Node->tag == astTOP)
        Value = emitterTOP(ctx, Node);

    else if (Node->tag == astIndex)
        Value = emitterIndex(ctx, Node);

    else if (Node->tag == astCall)
        Value = emitterCall(ctx, Node);

    else if (Node->tag == astCast)
        Value = emitterValue(ctx, Node->r, Dest);

    else if (Node->tag == astSizeof)
        Value = emitterSizeof(ctx, Node);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            Value = emitterSymbol(ctx, Node);

        else if (Node->litTag == literalCompound)
            Value = emitterCompoundLiteral(ctx, Node);

        else
            Value = emitterLiteral(ctx, Node);

    } else {
        debugErrorUnhandled("emitterValue", "AST tag", astTagGetStr(Node->tag));
        return Dest;
    }

    /*Put it where requested*/

    /*If they haven't specifically asked for the reference as memory
      then they're unaware it's held as a reference at all
      so make it a plain ol' value*/
    if (Value.tag == operandMemRef && Dest.tag != operandMem) {
        operand nValue = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->Asm, nValue, Value);
        operandFree(Value);
        Value = nValue;
    }

    if (Dest.tag != operandUndefined) {
        if (Dest.tag != Value.tag) {
            if (Dest.tag == operandFlags) {
                asmCompare(ctx->Asm, Value, operandCreateLiteral(0));
                operandFree(Value);

                Dest = operandCreateFlags(conditionEqual);

            } else if (Dest.tag == operandReg) {
                /*If a specific register wasn't requested, allocate one*/
                if (Dest.base == regUndefined)
                    Dest.base = regAlloc(typeGetSize(ctx->arch, Node->dt));

                asmMove(ctx->Asm, Dest, Value);
                operandFree(Value);

            } else if (Dest.tag == operandMem) {
                if (Value.tag == operandMemRef) {
                    Dest = Value;
                    Dest.tag = operandMem;

                } else
                    printf("emitterValue(): unable to convert non lvalue operand tag, %s.\n", operandTagGetStr(Value.tag));

            } else if (Dest.tag == operandStack) {
                asmPush(ctx->Asm, Value);

                if (Value.tag == operandMem)
                    Dest.size = operandGetSize(ctx->arch, Value);

                else
                    Dest.size = ctx->arch->wordsize;

                operandFree(Value);

            } else if (Value.tag == operandUndefined)
                printf("emitterValue(): expected value, void given.\n");

            else {
                debugErrorUnhandled("emitterValue", "operand tag", operandTagGetStr(Value.tag));
                debugErrorUnhandled("emitterValue", "operand tag", operandTagGetStr(Dest.tag));
            }

        } else if (   Dest.tag == operandReg
                   && Dest.base != Value.base
                   && Dest.base != regUndefined) {
            asmMove(ctx->Asm, Dest, Value);
            operandFree(Value);

        } else
            Dest = Value;

    } else
        Dest = Value;

    return Dest;
}

static operand emitterBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("BOP");

    operand L, R, Value;

    if (!strcmp(Node->o, ".")) {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, operandCreate(operandMem));
        asmLeave(ctx->Asm);

        Value.offset += Node->symbol->offset;
        Value.size = typeGetSize(ctx->arch, Node->symbol->dt);

    } else if (!strcmp(Node->o, "->")) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, operandCreateReg(regUndefined));
        asmLeave(ctx->Asm);

        Value = operandCreateMem(L.base,
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch, Node->dt));

    } else if (   !strcmp(Node->o, "==")
               || !strcmp(Node->o, "!=")
               || !strcmp(Node->o, "<")
               || !strcmp(Node->o, "<=")
               || !strcmp(Node->o, ">" )
               || !strcmp(Node->o, ">=")) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, operandCreate(operandUndefined));
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        Value = operandCreateFlags(conditionNegate(conditionFromStr(Node->o)));
        asmCompare(ctx->Asm, L, R);
        operandFree(L);
        operandFree(R);

    } else if (!strcmp(Node->o, ",")) {
        asmEnter(ctx->Asm);
        operandFree(emitterValue(ctx, Node->l, operandCreate(operandUndefined)));
        Value = R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

    } else {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, operandCreateReg(regUndefined));
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        boperation bop = !strcmp(Node->o, "+") ? bopAdd :
                         !strcmp(Node->o, "-") ? bopSub :
                         !strcmp(Node->o, "*") ? bopMul :
                         !strcmp(Node->o, "&") ? bopBitAnd :
                         !strcmp(Node->o, "|") ? bopBitOr :
                         !strcmp(Node->o, "^") ? bopBitXor :
                         !strcmp(Node->o, ">>") ? bopShR :
                         !strcmp(Node->o, "<<") ? bopShL : bopUndefined;

        if (bop)
            asmBOP(ctx->Asm, bop, L, R);

        else
            debugErrorUnhandled("emitterBOP", "operator", Node->o);

        operandFree(R);
    }

    debugLeave();

    return Value;
}

static operand emitterAssignmentBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("AssignnentBOP");

    asmEnter(ctx->Asm);
    operand Value, R = emitterValue(ctx, Node->r, operandCreate(operandUndefined)),
                   L = emitterValue(ctx, Node->l, operandCreate(operandMem));
    asmLeave(ctx->Asm);

    boperation bop = !strcmp(Node->o, "+=") ? bopAdd :
                     !strcmp(Node->o, "-=") ? bopSub :
                     !strcmp(Node->o, "*=") ? bopMul :
                     !strcmp(Node->o, "&=") ? bopBitAnd :
                     !strcmp(Node->o, "|=") ? bopBitOr :
                     !strcmp(Node->o, "^=") ? bopBitXor :
                     !strcmp(Node->o, ">>=") ? bopShR :
                     !strcmp(Node->o, "<<=") ? bopShL : bopUndefined;

    if (!strcmp(Node->o, "=")) {
        Value = R;
        asmMove(ctx->Asm, L, R);
        operandFree(L);

    } else if (bop != bopUndefined) {
        Value = L;
        asmBOP(ctx->Asm, bop, L, R);
        operandFree(R);

    } else
        debugErrorUnhandled("emitterAssignmentBOP", "operator", Node->o);

    debugLeave();

    return Value;
}

static operand emitterLogicalBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("LogicalBOP");

    /*Label to jump to if circuit gets shorted*/
    operand ShortLabel = labelCreate(labelUndefined);

    /*Left*/
    asmEnter(ctx->Asm);
    operand L = emitterValue(ctx, Node->l, operandCreate(operandFlags));
    asmLeave(ctx->Asm);

    /*Set up the short circuit value*/
    operand Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
    asmMove(ctx->Asm, Value, operandCreateLiteral(!strcmp(Node->o, "&&") ? 0 : 1));

    /*Check initial condition*/

    if (!strcmp(Node->o, "||"))
        L.condition = conditionNegate(L.condition);

    asmBranch(ctx->Asm, L, ShortLabel);

    /*Right*/
    asmEnter(ctx->Asm);
    operand R = emitterValue(ctx, Node->r, operandCreate(operandFlags));
    asmLeave(ctx->Asm);

    if (!strcmp(Node->o, "&&"))
        R.condition = conditionNegate(R.condition);

    asmConditionalMove(ctx->Asm, R, Value, operandCreateLiteral(!strcmp(Node->o, "||") ? 0 : 1));
    asmLabel(ctx->Asm, ShortLabel);

    debugLeave();

    return Value;
}

static operand emitterUOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("UOP");

    operand R, Value;

    if (   !strcmp(Node->o, "++")
        || !strcmp(Node->o, "--")) {
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

        operandFree(R);

    } else if (   !strcmp(Node->o, "-")
               || !strcmp(Node->o, "~")
               || !strcmp(Node->o, "!")) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        asmLeave(ctx->Asm);

        if (!strcmp(Node->o, "!")) {
            asmCompare(ctx->Asm, R, operandCreateLiteral(0));
            Value = operandCreateFlags(conditionNotEqual);
            operandFree(R);

        } else {
            asmUOP(ctx->Asm, !strcmp(Node->o, "-") ? uopNeg : uopBitwiseNot, R);
            Value = R;
        }

    } else if (!strcmp(Node->o, "*")) {
        operand Ptr = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        Value = operandCreateMem(Ptr.base, 0, typeGetSize(ctx->arch, Node->dt));

    } else if (!strcmp(Node->o, "&")) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreate(operandMem));
        asmLeave(ctx->Asm);

        Value = operandCreateReg(regAlloc(ctx->arch->wordsize));

        asmEvalAddress(ctx->Asm, Value, R);
        operandFree(R);

    } else {
        debugErrorUnhandled("emitterUOP", "operator", Node->o);
        Value = operandCreateInvalid();
    }

    debugLeave();

    return Value;
}

static operand emitterTOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("TOP");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);
    operand Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));

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

static operand emitterIndex (emitterCtx* ctx, const ast* Node) {
    debugEnter("Index");

    operand L, R, Value;

    /*Is it an array? Are we directly offsetting the address*/
    if (typeIsArray(Node->l->dt)) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, operandCreate(operandMem));
        R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);

        if (R.tag == operandLiteral) {
            Value = L;
            Value.offset += typeGetSize(ctx->arch, Node->l->dt->base) * R.literal;

        } else if (R.tag == operandReg) {
            Value = L;
            Value.index = R.base;
            Value.factor = typeGetSize(ctx->arch, Node->l->dt->base);

        } else {
            operand index = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->r->dt)));
            asmMove(ctx->Asm, index, R);
            operandFree(R);

            Value = L;
            Value.index = index.base;
            Value.factor = typeGetSize(ctx->arch, Node->l->dt->base);
        }

        Value.size = typeGetSize(ctx->arch, Node->dt);

    /*Is it instead a pointer? Get value and offset*/
    } else /*if (typeIsPtr(Node->l->dt)*/ {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(typeGetSize(ctx->arch, Node->l->dt->base)));
        asmLeave(ctx->Asm);

        L = emitterValue(ctx, Node->l, operandCreate(operandUndefined));

        asmBOP(ctx->Asm, bopAdd, R, L);
        operandFree(L);

        Value = operandCreateMem(R.base, 0, typeGetSize(ctx->arch, Node->dt));
    }

    debugLeave();

    return Value;
}

static operand emitterCall (emitterCtx* ctx, const ast* Node) {
    debugEnter("Call");

    operand Value;

    /*If larger than a word, the return will be passed in a temporary position
      (stack) allocated by the caller.*/
    bool retInTemp = typeGetSize(ctx->arch, Node->dt) > ctx->arch->wordsize;
    int tempWords = 0;

    if (retInTemp) {
        /*Allocate the temporary space (rounded up to the nearest word)*/
        tempWords = (typeGetSize(ctx->arch, Node->dt)-1)/ctx->arch->wordsize + 1;
        asmPushN(ctx->Asm, tempWords);
    }

    /*Save the general registers in use*/
    for (int r = regRAX; r <= regR15; r++)
        if (regIsUsed(r))
            asmPush(ctx->Asm, operandCreateReg(&regs[r]));

    int argSize = 0;

    /*Push the args on backwards (cdecl)*/
    for (ast* Current = Node->lastChild;
         Current;
         Current = Current->prevSibling) {
        operand Arg = emitterValue(ctx, Current, operandCreate(operandStack));
        argSize += Arg.size;
    }

    /*Pass on the reference to the temporary return space
      Last, so that a varargs fn can still locate it*/
    if (retInTemp) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->Asm, intermediate, operandCreateMem(&regs[regRSP], argSize, ctx->arch->wordsize));
        asmPush(ctx->Asm, intermediate);
        operandFree(intermediate);
    }

    /*Get the address (usually, as a label) of the proc*/
    operand Proc = emitterValue(ctx, Node->l, operandCreate(operandUndefined));
    asmCall(ctx->Asm, Proc);
    operandFree(Proc);

    if (!typeIsVoid(Node->dt)) {
        int size = retInTemp ? ctx->arch->wordsize : typeGetSize(ctx->arch, Node->dt);

        /*If RAX is already in use (currently backed up to the stack), relocate the
          return value to another free reg before RAX's value is restored.*/
        if (regIsUsed(regRAX)) {
            Value = operandCreateReg(regAlloc(size));
            int tmp = regs[regRAX].allocatedAs;
            regs[regRAX].allocatedAs = size;
            asmMove(ctx->Asm, Value, operandCreateReg(&regs[regRAX]));
            regs[regRAX].allocatedAs = tmp;

        } else
            Value = operandCreateReg(regRequest(regRAX, size));

        /*The temporary's pointer is returned to us*/
        if (retInTemp)
            Value = operandCreateMem(Value.base, 0, typeGetSize(ctx->arch, Node->dt));

    } else
        Value = operandCreateVoid();

    /*Pop the args (and temporary return space)*/
    asmPopN(ctx->Asm, argSize/ctx->arch->wordsize + tempWords);

    /*Restore the saved registers (backwards as stacks are LIFO)*/
    for (regIndex r = regR15; r >= regRAX; r--)
        /*Attempt to restore all but the one we just allocated for the ret value*/
        if (regIsUsed(r) && &regs[r] != Value.base)
            asmPop(ctx->Asm, operandCreateReg(&regs[r]));

    debugLeave();

    return Value;
}

static operand emitterSizeof (emitterCtx* ctx, const ast* Node) {
    debugEnter("Sizeof");

    const type* DT = Node->r->dt;
    operand Value = operandCreateLiteral(typeGetSize(ctx->arch, DT));

    debugLeave();

    return Value;
}

static operand emitterSymbol (emitterCtx* ctx, const ast* Node) {
    debugEnter("Symbol");

    operand Value = operandCreate(operandUndefined);

    if (typeIsFunction(Node->symbol->dt))
        Value = Node->symbol->label;

    else /*var or param*/ {
        /*Enum constant*/
        if (Node->symbol->tag == symEnumConstant)
            Value = operandCreateLiteral(Node->symbol->constValue);

        /*Array*/
        else if (typeIsArray(Node->symbol->dt))
            Value = operandCreateMemRef(&regs[regRBP],
                                        Node->symbol->offset,
                                        typeGetSize(ctx->arch, Node->symbol->dt->base));

        /*Regular stack allocated var/param*/
        else
            Value = operandCreateMem(&regs[regRBP],
                                     Node->symbol->offset,
                                     typeGetSize(ctx->arch, Node->symbol->dt));
    }

    debugLeave();

    return Value;
}

static operand emitterLiteral (emitterCtx* ctx, const ast* Node) {
    debugEnter("Literal");

    operand Value;

    if (Node->litTag == literalInt)
        Value = operandCreateLiteral(*(int*) Node->literal);

    else if (Node->litTag == literalChar)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else if (Node->litTag == literalBool)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else if (Node->litTag == literalStr) {
        Value = labelCreate(labelROData);
        asmStringConstant(ctx->Asm, Value, (char*) Node->literal);

    } else {
        debugErrorUnhandled("emitterLiteral", "literal tag", literalTagGetStr(Node->litTag));
        Value = operandCreateInvalid();
    }

    debugLeave();

    return Value;
}

static operand emitterCompoundLiteral (emitterCtx* ctx, const ast* Node) {
    debugEnter("CompoundLiteral");

    operand Value = operandCreateMem(&regs[regRBP],
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch, Node->symbol->dt));

    emitterInitOrCompoundLiteral(ctx, Node, Value);

    debugLeave();

    return Value;
}

void emitterInitOrCompoundLiteral (emitterCtx* ctx, const ast* Node, operand base) {
    debugEnter("InitOrCompoundLiteral");

    /*Struct initialization*/
    if (Node->dt->tag == typeBasic && Node->dt->basic->tag == symStruct) {
        sym* structSym = Node->dt->basic;

        ast* value;
        sym* field;

        /*For every field*/
        for (value = Node->firstChild, field = structSym->firstChild;
             value && field;
             value = value->nextSibling, field = field->nextSibling) {
            /*Prepare the left operand*/
            operand L = base;
            L.size = typeGetSize(ctx->arch, field->dt);
            L.offset += field->offset;

            /*Recursive initialization*/
            if (value->tag == astLiteral && value->litTag == literalInit) {
                emitterInitOrCompoundLiteral(ctx, value, L);

            /*Regular value*/
            } else {
                asmEnter(ctx->Asm);
                operand R = emitterValue(ctx, value, operandCreate(operandUndefined));
                asmLeave(ctx->Asm);

                asmMove(ctx->Asm, L, R);
                operandFree(R);
            }
        }

    /*Array initialization*/
    } else if (typeIsArray(Node->dt)) {
        int elementSize = typeGetSize(ctx->arch, Node->dt->base);
        operand L = base;
        L.size = elementSize;

        /*For every element*/
        for (ast* Current = Node->firstChild;
             Current;
             Current = Current->nextSibling) {
            asmEnter(ctx->Asm);
            operand R = emitterValue(ctx, Current, operandCreate(operandUndefined));
            asmLeave(ctx->Asm);

            asmMove(ctx->Asm, L, R);
            L.offset += elementSize;
            operandFree(R);
        }

    /*Scalar*/
    } else {
        asmEnter(ctx->Asm);
        operand R = emitterValue(ctx, Node->firstChild, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);
        asmMove(ctx->Asm, base, R);
        operandFree(R);
    }

    debugLeave();
}
