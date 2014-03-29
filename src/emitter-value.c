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

#include "stdio.h"
#include "stdlib.h"

static operand emitterValueImpl (emitterCtx* ctx, const ast* Node,
                                 emitterRequest request, const operand* suggestion);

static operand emitterBOP (emitterCtx* ctx, const ast* Node);
static operand emitterAssignmentBOP (emitterCtx* ctx, const ast* Node);
static operand emitterLogicalBOP (emitterCtx* ctx, const ast* Node);
static operand emitterLogicalBOPImpl (emitterCtx* ctx, const ast* Node, operand ShortLabel, operand* Value);

static operand emitterUOP (emitterCtx* ctx, const ast* Node);
static operand emitterTOP (emitterCtx* ctx, const ast* Node, const operand* suggestion);
static operand emitterIndex (emitterCtx* ctx, const ast* Node);
static operand emitterCall (emitterCtx* ctx, const ast* Node);
static operand emitterCast (emitterCtx* ctx, const ast* Node);
static operand emitterSizeof (emitterCtx* ctx, const ast* Node);
static operand emitterSymbol (emitterCtx* ctx, const ast* Node);
static operand emitterLiteral (emitterCtx* ctx, const ast* Node);
static operand emitterCompoundLiteral (emitterCtx* ctx, const ast* Node);

operand emitterValue (emitterCtx* ctx, const ast* Node, emitterRequest request) {
    return emitterValueImpl(ctx, Node, request, 0);
}

operand emitterValueSuggest (emitterCtx* ctx, const ast* Node, const operand* request) {
    return emitterValueImpl(ctx, Node, requestAny, request);
}

static operand emitterValueImpl (emitterCtx* ctx, const ast* Node,
                                 emitterRequest request, const operand* suggestion) {
    operand Value;

    /*Calculate the value*/

    if (Node->tag == astBOP) {
        if (opIsAssignment(Node->o))
            Value = emitterAssignmentBOP(ctx, Node);

        else if (opIsLogical(Node->o))
            Value = emitterLogicalBOP(ctx, Node);

        else
            Value = emitterBOP(ctx, Node);

    } else if (Node->tag == astUOP)
        Value = emitterUOP(ctx, Node);

    else if (Node->tag == astTOP)
        Value = emitterTOP(ctx, Node, suggestion);

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
    if (   Value.tag == operandMemRef
        && (request != requestMem && !(suggestion && suggestion->tag == operandMem))) {
        operand nValue = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->Asm, nValue, Value);
        operandFree(Value);
        Value = nValue;
    }

    operand Dest;

    if (suggestion) {
        if (!operandIsEqual(Value, *suggestion)) {
            asmMove(ctx->Asm, *suggestion, Value);
            operandFree(Value);
            Dest = *suggestion;

        } else
            Dest = Value;

    } else if (request == requestAny)
        Dest = Value;

    else if (request == requestReg) {
        if (Value.tag != operandReg){
            Dest = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
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

    if (Node->o == opMember) {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, requestMem);
        asmLeave(ctx->Asm);

        Value.offset += Node->symbol->offset;
        Value.size = typeGetSize(ctx->arch, Node->symbol->dt);

    } else if (Node->o == opMemberDeref) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, requestReg);
        asmLeave(ctx->Asm);

        Value = operandCreateMem(L.base,
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch, Node->dt));

    } else if (opIsEquality(Node->o) || opIsOrdinal(Node->o)) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, requestOperable);
        R = emitterValue(ctx, Node->r, requestOperable);
        asmLeave(ctx->Asm);

        Value = operandCreateFlags(conditionNegate(conditionFromOp(Node->o)));
        asmCompare(ctx->Asm, L, R);
        operandFree(L);
        operandFree(R);

    } else if (Node->o == opComma) {
        asmEnter(ctx->Asm);
        operandFree(emitterValue(ctx, Node->l, requestAny));
        Value = R = emitterValue(ctx, Node->r, requestAny);
        asmLeave(ctx->Asm);

    } else {
        asmEnter(ctx->Asm);
        Value = L = emitterValue(ctx, Node->l, requestReg);
        R = emitterValue(ctx, Node->r, requestOperable);
        asmLeave(ctx->Asm);

        boperation bop = Node->o == opAdd ? bopAdd :
                         Node->o == opSubtract ? bopSub :
                         Node->o == opMultiply ? bopMul :
                         Node->o == opBitwiseAnd ? bopBitAnd :
                         Node->o == opBitwiseOr ? bopBitOr :
                         Node->o == opBitwiseXor ? bopBitXor :
                         Node->o == opShr ? bopShR :
                         Node->o == opShl ? bopShL : bopUndefined;

        if (bop)
            asmBOP(ctx->Asm, bop, L, R);

        else
            debugErrorUnhandled("emitterBOP", "operator", opTagGetStr(Node->o));

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

    boperation bop = Node->o == opAddAssign ? bopAdd :
                     Node->o == opSubtractAssign ? bopSub :
                     Node->o == opMultiplyAssign ? bopMul :
                     Node->o == opBitwiseAndAssign ? bopBitAnd :
                     Node->o == opBitwiseOrAssign ? bopBitOr :
                     Node->o == opBitwiseXorAssign ? bopBitXor :
                     Node->o == opShrAssign ? bopShR :
                     Node->o == opShlAssign ? bopShL : bopUndefined;

    if (Node->o == opAssign) {
        Value = R;
        asmMove(ctx->Asm, L, R);
        operandFree(L);

    } else if (bop != bopUndefined) {
        Value = L;
        asmBOP(ctx->Asm, bop, L, R);
        operandFree(R);

    } else
        debugErrorUnhandled("emitterAssignmentBOP", "operator", opTagGetStr(Node->o));

    debugLeave();

    return Value;
}

static operand emitterLogicalBOP (emitterCtx* ctx, const ast* Node) {
    /*Label to jump to if circuit gets shorted*/
    operand ShortLabel = labelCreate(labelUndefined);

    operand Value;
    /*Move the default into Value, return the condition as flags*/
    operand R = emitterLogicalBOPImpl(ctx, Node, ShortLabel, &Value);

    if (Node->o == opLogicalAnd)
        R.condition = conditionNegate(R.condition);

    /*Move final value*/
    asmConditionalMove(ctx->Asm, R, Value, operandCreateLiteral(Node->o == opLogicalOr ? 0 : 1));

    /*If shorted come here leaving the default in Value*/
    asmLabel(ctx->Asm, ShortLabel);

    return Value;
}

static operand emitterLogicalBOPImpl (emitterCtx* ctx, const ast* Node, operand ShortLabel, operand* Value) {
    debugEnter("LogicalBOP");

    /*The job of this function is:
        - Move the default into Value (possibly passing the buck recursively)
        - Jump to ShortLabel depending on LHS
        - Return RHS as flags*/

    /*Value is where the default goes, L and R are the conditions*/

    operand L;

    /*If LHS is the same op, then the short value is the same
       => use our short label, their register*/
    if (Node->l->tag == astBOP && Node->l->o == Node->o)
        L = emitterLogicalBOPImpl(ctx, Node->l, ShortLabel, Value);

    else {
        /*Left*/
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, requestFlags);
        asmLeave(ctx->Asm);

        /*Set up the short circuit value*/
        *Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
        asmMove(ctx->Asm, *Value, operandCreateLiteral(Node->o == opLogicalAnd ? 0 : 1));
    }

    /*Check initial condition*/

    if (Node->o == opLogicalOr)
        L.condition = conditionNegate(L.condition);

    asmBranch(ctx->Asm, L, ShortLabel);

    /*Right*/
    asmEnter(ctx->Asm);
    operand R = emitterValue(ctx, Node->r, requestFlags);
    asmLeave(ctx->Asm);

    debugLeave();

    return R;
}

static operand emitterUOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("UOP");

    operand R, Value;

    if (   Node->o == opPostIncrement || Node->o == opPostDecrement
        || Node->o == opPreIncrement || Node->o == opPreDecrement) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestMem);
        asmLeave(ctx->Asm);

        bool post = Node->o == opPostIncrement || Node->o == opPostDecrement;

        /*Post ops: save a copy before inc/dec*/
        if (post) {
            Value = operandCreateReg(regAlloc(operandGetSize(ctx->arch, R)));
            asmMove(ctx->Asm, Value, R);

        } else
            Value = R;

        uoperation op =   Node->o == opPostIncrement || Node->o == opPreIncrement
                        ? uopInc : uopDec;

        asmUOP(ctx->Asm, op, R);

        if (post)
            operandFree(R);

    } else if (   Node->o == opNegate
               || Node->o == opBitwiseNot || Node->o == opLogicalNot) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestReg);
        asmLeave(ctx->Asm);

        if (Node->o == opLogicalNot) {
            asmCompare(ctx->Asm, R, operandCreateLiteral(0));
            Value = operandCreateFlags(conditionNotEqual);
            operandFree(R);

        } else {
            asmUOP(ctx->Asm, Node->o == opNegate ? uopNeg : uopBitwiseNot, R);
            Value = R;
        }

    } else if (Node->o == opDeref) {
        operand Ptr = emitterValue(ctx, Node->r, requestReg);
        Value = operandCreateMem(Ptr.base, 0, typeGetSize(ctx->arch, Node->dt));

    } else if (Node->o == opAddressOf) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestMem);
        asmLeave(ctx->Asm);

        Value = operandCreateReg(regAlloc(ctx->arch->wordsize));

        asmEvalAddress(ctx->Asm, Value, R);
        operandFree(R);

    } else {
        debugErrorUnhandled("emitterUOP", "operator", opTagGetStr(Node->o));
        Value = operandCreateInvalid();
    }

    debugLeave();

    return Value;
}

static operand emitterTOP (emitterCtx* ctx, const ast* Node, const operand* suggestion) {
    debugEnter("TOP");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);

    asmBranch(ctx->Asm,
              emitterValue(ctx, Node->firstChild, requestFlags),
              ElseLabel);

    /*Ask for LHS to go in a reg, or the suggestion. This becomes our return*/
    operand Value = emitterValueImpl(ctx, Node->l, requestReg, suggestion);

    asmComment(ctx->Asm, "");
    asmJump(ctx->Asm, EndLabel);
    asmLabel(ctx->Asm, ElseLabel);

    /*Move RHS into our reg*/
    emitterValueSuggest(ctx, Node->r, &Value);

    asmLabel(ctx->Asm, EndLabel);

    debugLeave();

    return Value;
}

static operand emitterGetInReg (emitterCtx* ctx, operand src, int size) {
    if (src.tag == operandReg)
        return src;

    operand dest = operandCreateReg(regAlloc(size));
    asmMove(ctx->Asm, dest, src);
    operandFree(src);
    return dest;
}

static operand emitterIndex (emitterCtx* ctx, const ast* Node) {
    debugEnter("Index");

    operand L, R, Value;

    int size = typeGetSize(ctx->arch, Node->dt);

    /*Is it an array? Are we directly offsetting the address*/
    if (typeIsArray(Node->l->dt)) {
        asmEnter(ctx->Asm);
        L = emitterValue(ctx, Node->l, requestMem);
        R = emitterValue(ctx, Node->r, requestOperable);
        asmLeave(ctx->Asm);

        /*Just a constant? Add to the offset*/
        if (R.tag == operandLiteral) {
            Value = L;
            Value.offset += size*R.literal;

        /*Has an index but factor matches? Add to the index*/
        } else if (L.index && L.factor == size) {
            asmBOP(ctx->Asm, bopAdd, operandCreateReg(L.index), R);
            operandFree(R);
            Value = L;

        } else {
            R = emitterGetInReg(ctx, R, typeGetSize(ctx->arch, Node->r->dt));

            /*If L doesn't have an index, we can use it directly*/
            if (!L.index) {
                Value = L;
                Value.tag = operandMem;

            /*Evaluate the address, create use result as base of new operand*/
            } else {
                Value = operandCreateMem(regAlloc(ctx->arch->wordsize), 0, size);
                asmEvalAddress(ctx->Asm, operandCreateReg(Value.base), L);
                operandFree(L);
            }

            Value.index = R.base;

            /*Use a convenient factor if the result too is an array*/
            if (typeIsArray(Node->dt)) {
                int baseSize = typeGetSize(ctx->arch, typeGetBase(Node->dt));

                Value.factor =   baseSize == 1 || baseSize == 2 || baseSize == 4 || baseSize == 8
                               ? baseSize : 1;

            /*Or the size itself*/
            } else if (size == 1 || size == 2 || size == 4 || size == 8)
                Value.factor = size;

            /*Just have to multiply it anyway*/
            else
                Value.factor = size % 4 == 0 ? size/4 : 1;

            int multiplier = size/Value.factor;

            if (multiplier != 1)
                asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(multiplier));
        }

        Value.size = size;

    /*Is it instead a pointer? Get value and offset*/
    } else /*if (typeIsPtr(Node->l->dt)*/ {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, requestReg);
        asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(size));
        asmLeave(ctx->Asm);

        L = emitterValue(ctx, Node->l, requestOperable);

        asmBOP(ctx->Asm, bopAdd, R, L);
        operandFree(L);

        Value = operandCreateMem(R.base, 0, size);
    }

    debugLeave();

    return Value;
}

static operand emitterCall (emitterCtx* ctx, const ast* Node) {
    debugEnter("Call");

    operand Value;

    /*Caller save registers: only if in use*/
    for (int i = 0; i < ctx->arch->callerSavedRegs.length; i++) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->callerSavedRegs, i);

        if (regIsUsed(r))
            asmSaveReg(ctx->Asm, r);
    }

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
    for (int i = ctx->arch->callerSavedRegs.length-1; i >= 0 ; i--) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->callerSavedRegs, i);

        if (regIsUsed(r) && regGet(r) != Value.base)
            asmRestoreReg(ctx->Asm, r);
    }

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
