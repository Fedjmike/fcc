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
static operand emitterCast (emitterCtx* ctx, const ast* Node);
static operand emitterSizeof (emitterCtx* ctx, const ast* Node);
static operand emitterSymbol (emitterCtx* ctx, const ast* Node);
static operand emitterLiteral (emitterCtx* ctx, const ast* Node);
static operand emitterCompoundLiteral (emitterCtx* ctx, const ast* Node);

operand emitterValue (emitterCtx* ctx, const ast* Node, emitterRequest request) {
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
        Value = emitterCast(ctx, Node);

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
        return operandCreate(operandUndefined);
    }

    /*Put it where requested*/

    /*If they haven't specifically asked for the reference as memory
      then they're unaware it's held as a reference at all
      so make it a plain ol' value*/
    if (Value.tag == operandMemRef && request != requestMem) {
        operand nValue = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->Asm, nValue, Value);
        operandFree(Value);
        Value = nValue;
    }

    operand Dest;

    if (request == requestAny)
        Dest = Value;

    else if (request == requestReg) {
        /*Not in a reg, or wrong one*/
        if (   Value.tag != operandReg
            ){//|| (request.specific != regUndefined && request.specific != Value.base)) {
            Dest = operandCreateReg(  //Value.tag == operandReg
                                    //? request.specific
                                    regAlloc(typeGetSize(ctx->arch, Node->dt)));

            asmMove(ctx->Asm, Dest, Value);
            operandFree(Value);

        } else
            Dest = Value;

    } else if (request == requestMem) {
        if (Value.tag != operandMem && Value.tag != operandMemRef)
            debugError("emitterValue", "unable to convert non lvalue operand tag, %s", operandTagGetStr(Value.tag));

        Dest = Value;

    } else if (request == requestOperable) {
        if (   Value.tag == operandLiteral || Value.tag == operandReg
            || Value.tag == operandMem)
            Dest = Value;

        else if (    Value.tag == operandMemRef || Value.tag == operandFlags
                  || Value.tag == operandLabel || Value.tag == operandLabelMem
                  || Value.tag == operandLabelOffset) {
            Dest = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
            asmMove(ctx->Asm, Dest, Value);
            operandFree(Value);

        } else
            debugError("emitterValue", "unable to convert %s to operable", operandTagGetStr(Value.tag));

    } else if (request == requestFlags) {
        if (Value.tag != operandFlags) {
            asmCompare(ctx->Asm, Value, operandCreateLiteral(0));
            operandFree(Value);

            Dest = operandCreateFlags(conditionEqual);

        } else
            Dest = Value;

    } else if (request == requestStack) {
        if (Value.tag != operandStack) {
            Dest = operandCreate(operandStack);

            if (Value.tag == operandMem)
                Dest.size = operandGetSize(ctx->arch, Value);

            else
                Dest.size = ctx->arch->wordsize;

            asmPush(ctx->Asm, Value);
            operandFree(Value);

        } else
            Dest = Value;

    }

    return Dest;
}

static operand emitterBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("BOP");

    operand L, R, Value;

    if (!strcmp(Node->o, ".")) {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, requestMem);
        asmLeave(ctx->Asm);

        Value.offset += Node->symbol->offset;
        Value.size = typeGetSize(ctx->arch, Node->symbol->dt);

    } else if (!strcmp(Node->o, "->")) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, requestReg);
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
        L = emitterValue(ctx, Node->l, requestOperable);
        R = emitterValue(ctx, Node->r, requestOperable);
        asmLeave(ctx->Asm);

        Value = operandCreateFlags(conditionNegate(conditionFromStr(Node->o)));
        asmCompare(ctx->Asm, L, R);
        operandFree(L);
        operandFree(R);

    } else if (!strcmp(Node->o, ",")) {
        asmEnter(ctx->Asm);
        operandFree(emitterValue(ctx, Node->l, requestAny));
        Value = R = emitterValue(ctx, Node->r, requestAny);
        asmLeave(ctx->Asm);

    } else {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, requestReg);
        R = emitterValue(ctx, Node->r, requestOperable);
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
    operand Value, R = emitterValue(ctx, Node->r, requestOperable),
                   L = emitterValue(ctx, Node->l, requestMem);
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
    operand L = emitterValue(ctx, Node->l, requestFlags);
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
    operand R = emitterValue(ctx, Node->r, requestFlags);
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
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestMem);
        asmLeave(ctx->Asm);

        Value = operandCreateReg(regAlloc(operandGetSize(ctx->arch, R)));
        asmMove(ctx->Asm, Value, R);

        if (!strcmp(Node->o, "++"))
            asmUOP(ctx->Asm, uopInc, R);

        else /*if (!strcmp(Node->o, "--"))*/
            asmUOP(ctx->Asm, uopDec, R);

        operandFree(R);

    } else if (   !strcmp(Node->o, "-")
               || !strcmp(Node->o, "~")
               || !strcmp(Node->o, "!")) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestReg);
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
        operand Ptr = emitterValue(ctx, Node->r, requestReg);
        Value = operandCreateMem(Ptr.base, 0, typeGetSize(ctx->arch, Node->dt));

    } else if (!strcmp(Node->o, "&")) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestMem);
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

    asmBranch(ctx->Asm,
              emitterValue(ctx, Node->firstChild, requestFlags),
              ElseLabel);

    operand Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));

    {
        operand tmp = emitterValue(ctx, Node->l, requestAny);
        asmMove(ctx->Asm, Value, tmp);
        operandFree(tmp);
    }

    asmComment(ctx->Asm, "");
    asmJump(ctx->Asm, EndLabel);
    asmLabel(ctx->Asm, ElseLabel);

    {
        operand tmp = emitterValue(ctx, Node->r, requestAny);
        asmMove(ctx->Asm, Value, tmp);
        operandFree(tmp);
    }

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
        L = emitterValue(ctx, Node->l, requestMem);
        R = emitterValue(ctx, Node->r, requestOperable);
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
        R = emitterValue(ctx, Node->r, requestReg);
        asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(typeGetSize(ctx->arch, Node->l->dt->base)));
        asmLeave(ctx->Asm);

        L = emitterValue(ctx, Node->l, requestOperable);

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

    /*Save the general registers in use*/
    for (int r = regRAX; r <= regR15; r++)
        if (regIsUsed(r))
            asmPush(ctx->Asm, operandCreateReg(&regs[r]));

    int argSize = 0;

    /*If larger than a word, the return will be passed in a temporary position
      (stack) allocated by the caller.*/
    bool retInTemp = typeGetSize(ctx->arch, Node->dt) > ctx->arch->wordsize;
    int tempWords = 0;

    if (retInTemp) {
        /*Allocate the temporary space (rounded up to the nearest word)*/
        tempWords = (typeGetSize(ctx->arch, Node->dt)-1)/ctx->arch->wordsize + 1;
        asmPushN(ctx->Asm, tempWords);
    }

    /*Push the args on backwards (cdecl)*/
    for (ast* Current = Node->lastChild;
         Current;
         Current = Current->prevSibling) {
        operand Arg = emitterValue(ctx, Current, requestStack);
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
    operand Proc = emitterValue(ctx, Node->l, requestAny);
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

    /*Pop the args (and temporary return space & reference)*/
    asmPopN(ctx->Asm, argSize/ctx->arch->wordsize + tempWords + (retInTemp ? 1 : 0));

    /*Restore the saved registers (backwards as stacks are LIFO)*/
    for (regIndex r = regR15; r >= regRAX; r--)
        /*Attempt to restore all but the one we just allocated for the ret value*/
        if (regIsUsed(r) && &regs[r] != Value.base)
            asmPop(ctx->Asm, operandCreateReg(&regs[r]));

    debugLeave();

    return Value;
}

static operand emitterCast (emitterCtx* ctx, const ast* Node) {
    debugEnter("Cast");

    operand R = emitterValue(ctx, Node->r, requestOperable);

    int from = typeGetSize(ctx->arch, Node->r->dt),
        to = typeGetSize(ctx->arch, Node->dt);

    if (from != to)
        R = (from < to ? asmWiden : asmNarrow)(ctx->Asm, R, to);

    debugLeave();

    return R;
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

    else {
        /*enum constant*/
        if (Node->symbol->tag == symEnumConstant)
            Value = operandCreateLiteral(Node->symbol->constValue);

        /*array*/
        else if (typeIsArray(Node->symbol->dt))
            Value = operandCreateMemRef(&regs[regRBP],
                                        Node->symbol->offset,
                                        typeGetSize(ctx->arch, Node->symbol->dt->base));

        /*regular variable*/
        else {
            int size = typeGetSize(ctx->arch, Node->symbol->dt);

            if (Node->symbol->storage == storageAuto)
                Value = operandCreateMem(&regs[regRBP], Node->symbol->offset, size);

            else if (   Node->symbol->storage == storageStatic
                     || Node->symbol->storage == storageExtern) {
                Value = Node->symbol->label;
                Value = operandCreateLabelMem(Value.label, size);

            } else
                debugErrorUnhandled("emitterSymbol", "storage tag", storageTagGetStr(Node->symbol->storage));
        }

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
        Value.tag = operandLabelOffset;
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
    if (typeIsStruct(Node->dt)) {
        const sym* structSym = Node->dt->basic;

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

            if (value->tag == astEmpty)
                ;

            /*Recursive initialization*/
            else if (value->tag == astLiteral && value->litTag == literalInit) {
                emitterInitOrCompoundLiteral(ctx, value, L);

            /*Regular value*/
            } else {
                asmEnter(ctx->Asm);
                operand R = emitterValue(ctx, value, requestOperable);
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
            if (Current->tag == astEmpty)
                ;

            /*Recursive initialization*/
            else if (Current->tag == astLiteral && Current->litTag == literalInit)
                emitterInitOrCompoundLiteral(ctx, Current, L);

            /*Regular value*/
            else {
                asmEnter(ctx->Asm);
                operand R = emitterValue(ctx, Current, requestOperable);
                asmLeave(ctx->Asm);

                asmMove(ctx->Asm, L, R);
                operandFree(R);
            }

            L.offset += elementSize;
        }

    /*Scalar*/
    } else {
        asmEnter(ctx->Asm);
        operand R = emitterValue(ctx, Node->firstChild, requestOperable);
        asmLeave(ctx->Asm);
        asmMove(ctx->Asm, base, R);
        operandFree(R);
    }

    debugLeave();
}
