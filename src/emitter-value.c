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

static operand emitterGetInReg (emitterCtx* ctx, operand src, int size);

/**
 * Forcibly allocate a register, saving its old value on the stack if need be.
 * @see emitterGiveBackReg()
 */
static operand emitterTakeReg (emitterCtx* ctx, regIndex r, int* oldSize, int newSize);
static void emitterGiveBackReg (emitterCtx* ctx, regIndex r, int oldSize);

static operand emitterValueImpl (emitterCtx* ctx, const ast* Node,
                                 emitterRequest request, const operand* suggestion);

static operand emitterBOP (emitterCtx* ctx, const ast* Node);
static operand emitterShiftBOP (emitterCtx* ctx, const ast* Node);
static operand emitterDivisionBOP (emitterCtx* ctx, const ast* Node);
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
static void emitterElementInit (emitterCtx* ctx, ast* Node, operand L);

operand emitterValue (emitterCtx* ctx, const ast* Node, emitterRequest request) {
    return emitterValueImpl(ctx, Node, request, 0);
}

operand emitterValueSuggest (emitterCtx* ctx, const ast* Node, const operand* request) {
    return emitterValueImpl(ctx, Node, requestAny, request);
}

static operand emitterValueImpl (emitterCtx* ctx, const ast* Node,
                                 emitterRequest request, const operand* suggestion) {
    asmEnter(ctx->Asm);

    operand Value;

    /*Calculate the value*/

    if (Node->tag == astBOP) {
        if (   Node->o == opDivide || Node->o == opDivideAssign
            || Node->o == opModulo || Node->o == opModuloAssign)
            Value = emitterDivisionBOP(ctx, Node);

        else if (   Node->o == opShl || Node->o == opShlAssign
                 || Node->o == opShr || Node->o == opShrAssign)
            Value = emitterShiftBOP(ctx, Node);

        else if (opIsAssignment(Node->o))
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
        debugErrorUnhandled("emitterValueImpl", "AST tag", astTagGetStr(Node->tag));
        Value = operandCreate(operandUndefined);
        request = requestAny;
    }

    /*Put it where requested*/

    /*Allow an array to decay to a pointer unless being used as an array*/
    if (Value.array && request != requestArray) {
        operand address = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->Asm, address, Value);
        operandFree(Value);
        Value = address;
    }

    operand Dest;

    if (suggestion) {
        if (!operandIsEqual(Value, *suggestion)) {
            asmMove(ctx->Asm, *suggestion, Value);
            operandFree(Value);
            Dest = *suggestion;

        } else
            Dest = Value;

    } else if (request == requestAny) {
        Dest = Value;

    /*Void: throw away result*/
    } else if (request == requestVoid) {
        operandFree(Value);

    /*Lvalue*/
    } else if (request == requestMem) {
        Dest = Value;

        if (Value.tag == operandLabelMem) {
            operand base = emitterGetInReg(ctx, Value, ctx->arch->wordsize);
            Dest = operandCreateMem(base.base, 0, typeGetSize(ctx->arch, Node->dt));

        } else if (Value.tag != operandMem)
            debugError("emitterValueImpl", "unable to convert non lvalue operand tag, %s", operandTagGetStr(Value.tag));

    /*Specific class of operand*/
    } else if (request == requestReg || request == requestRegOrMem || request == requestValue) {
        if (   Value.tag == operandReg
            || (Value.tag == operandMem && (request == requestRegOrMem || request == requestValue))
            || (Value.tag == operandLiteral && request == requestValue))
            Dest = Value;

        else {
            Dest = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
            asmMove(ctx->Asm, Dest, Value);
            operandFree(Value);
        }

    /*Array, from Mem or LabelMem*/
    } else if (request == requestArray) {
        Dest = Value;

        if (!Value.array) {
            debugError("emitterValueImpl", "unable to convert non array operand");
            reportOperand(ctx->arch, &Value);

        } else if (Value.tag != operandMem) {
            operand base = operandCreateReg(regAlloc(ctx->arch->wordsize));
            asmEvalAddress(ctx->Asm, base, Value);
            operandFree(Value);
            Dest = operandCreateMem(base.base, 0, typeGetSize(ctx->arch, Node->dt));
        }

    /*Compare against zero to get flags*/
    } else if (request == requestFlags) {
        if (Value.tag != operandFlags) {
            asmCompare(ctx->Asm, Value, operandCreateLiteral(0));
            operandFree(Value);

            Dest = operandCreateFlags(conditionEqual);

        } else
            Dest = Value;

    /*Push onto stack*/
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

    asmLeave(ctx->Asm);

    return Dest;
}

static operand emitterBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("BOP");

    operand L, R, Value;

    /* '.' */
    if (Node->o == opMember) {
        Value = L = emitterValue(ctx, Node->l, requestMem);
        Value.offset += Node->symbol->offset;
        Value.size = typeGetSize(ctx->arch, Node->symbol->dt);

    /* '->' */
    } else if (Node->o == opMemberDeref) {
        L = emitterValue(ctx, Node->l, requestReg);

        Value = operandCreateMem(L.base,
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch, Node->dt));

    /*Comparison operator*/
    } else if (opIsEquality(Node->o) || opIsOrdinal(Node->o)) {
        L = emitterValue(ctx, Node->l, requestRegOrMem);
        R = emitterValue(ctx, Node->r, requestValue);

        Value = operandCreateFlags(conditionNegate(conditionFromOp(Node->o)));
        asmCompare(ctx->Asm, L, R);
        operandFree(L);
        operandFree(R);

    /* ',' */
    } else if (Node->o == opComma) {
        operandFree(emitterValue(ctx, Node->l, requestAny));
        Value = R = emitterValue(ctx, Node->r, requestAny);

    /*Numeric operator*/
    } else {
        Value = L = emitterValue(ctx, Node->l, requestReg);
        R = emitterValue(ctx, Node->r, requestValue);

        boperation bop = Node->o == opAdd ? bopAdd :
                         Node->o == opSubtract ? bopSub :
                         Node->o == opMultiply ? bopMul :
                         Node->o == opBitwiseAnd ? bopBitAnd :
                         Node->o == opBitwiseOr ? bopBitOr :
                         Node->o == opBitwiseXor ? bopBitXor : bopUndefined;

        if (bop)
            asmBOP(ctx->Asm, bop, L, R);

        else
            debugErrorUnhandled("emitterBOP", "operator", opTagGetStr(Node->o));

        operandFree(R);
    }

    debugLeave();

    return Value;
}

static operand emitterTakeReg (emitterCtx* ctx, regIndex r, int* oldSize, int newSize) {
    if (regIsUsed(r)) {
        *oldSize = regs[r].allocatedAs;
        asmSaveReg(ctx->Asm, r);

    } else
        *oldSize = 0;

    regs[r].allocatedAs = newSize;
    return operandCreateReg(&regs[r]);
}

static void emitterGiveBackReg (emitterCtx* ctx, regIndex r, int oldSize) {
    regs[r].allocatedAs = oldSize;

    if (oldSize)
        asmRestoreReg(ctx->Asm, r);
}

static operand emitterDivisionBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("DivisionBOP");

    /*x86 is really non-orthogonal for division. EDX:EAX is treated as the LHS
      and then EDX stores the remainder and EAX the quotient*/

    bool isAssign = Node->o == opDivideAssign || Node->o == opModuloAssign,
         isModulo = Node->o == opModulo || Node->o == opModuloAssign;

    int raxOldSize, rdxOldSize;

    /*RAX: take, but save if necessary*/
    operand RAX = emitterTakeReg(ctx, regRAX, &raxOldSize, typeGetSize(ctx->arch, Node->l->dt));

    /*RDX: take, (save), clear*/
    operand RDX = emitterTakeReg(ctx, regRDX, &rdxOldSize, ctx->arch->wordsize);
    asmMove(ctx->Asm, RDX, operandCreateLiteral(0));

    /*RHS*/
    operand Value, L, R = emitterValue(ctx, Node->r, requestRegOrMem);

    /*LHS*/

    if (isAssign) {
        L = emitterValue(ctx, Node->l, requestMem);
        asmMove(ctx->Asm, RAX, L);

    } else
        L = emitterValueSuggest(ctx, Node->l, &RAX);

    /*The calculation*/
    asmDivision(ctx->Asm, R);
    operandFree(R);

    /*If the result reg was used before, move it to a new reg*/
    if ((isModulo ? rdxOldSize : raxOldSize) != 0) {
        Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
        asmMove(ctx->Asm, Value, isModulo ? RDX : RAX);

        /*Restore regs*/
        emitterGiveBackReg(ctx, regRDX, rdxOldSize);
        emitterGiveBackReg(ctx, regRAX, raxOldSize);

    } else {
        if (isModulo) {
            Value = RDX;
            emitterGiveBackReg(ctx, regRAX, raxOldSize);

        } else {
            Value = RAX;
            emitterGiveBackReg(ctx, regRDX, rdxOldSize);
        }
    }

    /*If an assignment, also move the result into memory*/
    if (isAssign) {
        asmMove(ctx->Asm, L, Value);
        operandFree(L);
    }

    debugLeave();

    return Value;
}

static operand emitterShiftBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("ShiftBOP");

    operand R;
    int rcxOldSize;

    /*Is the RHS an immediate?*/
    bool immediate =    Node->r->tag == astLiteral
                     && (   Node->r->litTag == literalInt || Node->r->litTag == literalChar
                         || Node->r->litTag == literalBool);

    /*Then use it directly*/
    if (immediate)
        R = emitterLiteral(ctx, Node->r);

    else {
        /*Shifting too is non-orthogonal. RHS goes in RCX, CL used as the operand.
          If RHS >= 2^8 or < 0, too bad.*/

        R = emitterTakeReg(ctx, regRCX, &rcxOldSize, typeGetSize(ctx->arch, Node->l->dt));
        emitterValueSuggest(ctx, Node->r, &R);

        /*Only CL*/
        R.base->allocatedAs = 1;
    }

    /*Assignment op? Then get the LHS as an lvalue, otherwise in a register*/
    emitterRequest Lrequest =   Node->o == opShlAssign || Node->o == opShrAssign
                              ? requestMem : requestReg;
    operand L = emitterValue(ctx, Node->l, Lrequest);

    /*Shift*/
    boperation op = Node->o == opShl || Node->o == opShlAssign ? bopShL : bopShR;
    asmBOP(ctx->Asm, op, L, R);

    /*Give back RCX*/
    if (!immediate)
        emitterGiveBackReg(ctx, regRCX, rcxOldSize);

    debugLeave();

    return L;
}

static operand emitterAssignmentBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("AssignnentBOP");

    /*Keep the left in memory so that the lvalue gets modified*/
    operand Value, R = emitterValue(ctx, Node->r, requestValue),
                   L = emitterValue(ctx, Node->l, requestMem);

    boperation bop = Node->o == opAddAssign ? bopAdd :
                     Node->o == opSubtractAssign ? bopSub :
                     Node->o == opMultiplyAssign ? bopMul :
                     Node->o == opBitwiseAndAssign ? bopBitAnd :
                     Node->o == opBitwiseOrAssign ? bopBitOr :
                     Node->o == opBitwiseXorAssign ? bopBitXor : bopUndefined;

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
    operand ShortLabel = asmCreateLabel(ctx->Asm, labelShortCircuit);

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
        L = emitterValue(ctx, Node->l, requestFlags);

        /*Set up the short circuit value*/
        *Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
        asmMove(ctx->Asm, *Value, operandCreateLiteral(Node->o == opLogicalAnd ? 0 : 1));
    }

    /*Check initial condition*/

    if (Node->o == opLogicalOr)
        L.condition = conditionNegate(L.condition);

    asmBranch(ctx->Asm, L, ShortLabel);

    /*Right*/
    operand R = emitterValue(ctx, Node->r, requestFlags);

    debugLeave();

    return R;
}

static operand emitterUOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("UOP");

    operand R, Value;

    /*Increment/decrement ops*/
    if (   Node->o == opPostIncrement || Node->o == opPostDecrement
        || Node->o == opPreIncrement || Node->o == opPreDecrement) {
        R = emitterValue(ctx, Node->r, requestMem);

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

    /*Numerical and logical ops*/
    } else if (   Node->o == opNegate || Node->o == opUnaryPlus
               || Node->o == opBitwiseNot || Node->o == opLogicalNot) {
        R = emitterValue(ctx, Node->r, requestReg);

        if (Node->o == opLogicalNot) {
            asmCompare(ctx->Asm, R, operandCreateLiteral(0));
            Value = operandCreateFlags(conditionNotEqual);
            operandFree(R);

        } else if (Node->o == opUnaryPlus) {
            Value = R;

        } else {
            asmUOP(ctx->Asm, Node->o == opNegate ? uopNeg : uopBitwiseNot, R);
            Value = R;
        }

    /*Deref*/
    } else if (Node->o == opDeref) {
        operand Ptr = emitterValue(ctx, Node->r, requestReg);
        Value = operandCreateMem(Ptr.base, 0, typeGetSize(ctx->arch, Node->dt));

    /*Address of*/
    } else if (Node->o == opAddressOf) {
        R = emitterValue(ctx, Node->r, requestMem);

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

    operand ElseLabel = asmCreateLabel(ctx->Asm, labelElse);
    operand EndLabel = asmCreateLabel(ctx->Asm, labelEndIf);

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
        L = emitterValue(ctx, Node->l, requestArray);
        R = emitterValue(ctx, Node->r, requestValue);

        /*Is the RHS just a constant? Add it to the offset*/
        if (R.tag == operandLiteral) {
            Value = L;
            Value.offset += size*R.literal;

        /*LHS has an index but factor matches? Add RHS to the index*/
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

            /*Evaluate the address of L, use the result as base of new operand*/
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
                Value.factor = size % 4 == 0 ? 4 : 1;

            int multiplier = size/Value.factor;

            if (multiplier != 1)
                asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(multiplier));
        }

        Value.size = size;
        Value.array = false;

    /*Is it instead a pointer? Get value and offset*/
    } else /*if (typeIsPtr(Node->l->dt)*/ {
        /* index = R*sizeof(*L) */
        R = emitterValue(ctx, Node->r, requestReg);
        asmBOP(ctx->Asm, bopMul, R, operandCreateLiteral(size));

        L = emitterValue(ctx, Node->l, requestValue);

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
    for (int i = 0; i < ctx->arch->scratchRegs.length; i++) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->scratchRegs, i);

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
    for (int i = ctx->arch->scratchRegs.length-1; i >= 0 ; i--) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->scratchRegs, i);

        if (regIsUsed(r) && regGet(r) != Value.base)
            asmRestoreReg(ctx->Asm, r);
    }

    debugLeave();

    return Value;
}

static operand emitterCast (emitterCtx* ctx, const ast* Node) {
    debugEnter("Cast");

    operand R = emitterValue(ctx, Node->r, requestValue);

    /*Widen or narrow, if needed*/

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

    /*function*/
    if (typeIsFunction(Node->symbol->dt))
        Value = operandCreateLabel(Node->symbol->label);

    /*enum constant*/
    else if (Node->symbol->tag == symEnumConstant)
        Value = operandCreateLiteral(Node->symbol->constValue);

    /*variable or param*/
    else {
        bool array = typeIsArray(Node->symbol->dt);
        int size = typeGetSize(ctx->arch,   array
                                          ? typeGetBase(Node->symbol->dt)
                                          : Node->symbol->dt);

        if (Node->symbol->storage == storageAuto)
            Value = operandCreateMem(&regs[regRBP], Node->symbol->offset, size);

        else if (   Node->symbol->storage == storageStatic
                   || Node->symbol->storage == storageExtern)
            Value = operandCreateLabelMem(Node->symbol->label, size);

        else
            debugErrorUnhandled("emitterSymbol", "storage tag", storageTagGetStr(Node->symbol->storage));

        Value.array = array;
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
        Value = asmCreateLabel(ctx->Asm, labelROData);
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

    operand Value = emitterSymbol(ctx, Node);
    emitterCompoundInit(ctx, Node, Value);

    debugLeave();

    return Value;
}

void emitterCompoundInit (emitterCtx* ctx, const ast* Node, operand base) {
    debugEnter("CompoundInit");

    /*Struct initialization*/
    if (typeIsStruct(Node->dt)) {
        const sym* record = typeGetBasic(Node->dt);

        ast* value;
        int n = 0;

        /*For every field*/
        for (value = Node->firstChild, n = 0;
             value && n < record->children.length;
             value = value->nextSibling, n++) {
            sym* field = vectorGet(&record->children, n);

            /*Prepare the left operand*/
            operand L = base;
            L.size = typeGetSize(ctx->arch, field->dt);
            L.offset += field->offset;

            emitterElementInit(ctx, value, L);
        }

    /*Array initialization*/
    } else if (typeIsArray(Node->dt)) {
        int elementSize = typeGetSize(ctx->arch, typeGetBase(Node->dt));
        operand L = base;
        L.size = elementSize;

        /*For every element*/
        for (ast* value = Node->firstChild;
             value;
             value = value->nextSibling) {
            emitterElementInit(ctx, value, L);
            L.offset += elementSize;
        }

    /*Scalar*/
    } else
        emitterValueSuggest(ctx, Node->firstChild, &base);
}

static void emitterElementInit (emitterCtx* ctx, ast* Node, operand L) {
    /*Skipped init*/
    if (Node->tag == astEmpty)
        ;

    /*Recursive initialization*/
    else if (Node->tag == astLiteral && Node->litTag == literalInit)
        emitterCompoundInit(ctx, Node, L);

    /*Regular value*/
    else
        emitterValueSuggest(ctx, Node, &L);
}
