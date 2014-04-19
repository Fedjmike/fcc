#include "../inc/emitter-value.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/architecture.h"
#include "../inc/ir.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"
#include "../inc/reg.h"

#include "../inc/emitter.h"
#include "../inc/emitter-helpers.h"

#include "stdio.h"
#include "stdlib.h"

static operand emitterValueImpl (emitterCtx* ctx, irBlock** block, const ast* Node,
                                 emitterRequest request, const operand* suggestion);

static operand emitterBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterShiftBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterDivisionBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterAssignmentBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterLogicalBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterLogicalBOPImpl (emitterCtx* ctx, irBlock** block, const ast* Node, irBlock* continuation, operand* Value);

static operand emitterUOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterTOP (emitterCtx* ctx, irBlock** block, const ast* Node, const operand* suggestion);
static operand emitterIndex (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterCall (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterCast (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterSizeof (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterSymbol (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterLiteral (emitterCtx* ctx, irBlock** block, const ast* Node);
static operand emitterCompoundLiteral (emitterCtx* ctx, irBlock** block, const ast* Node);
static void emitterElementInit (emitterCtx* ctx, irBlock** block, const ast* Node, operand L);

operand emitterValue (emitterCtx* ctx, irBlock** block, const ast* Node, emitterRequest request) {
    return emitterValueImpl(ctx, block, Node, request, 0);
}

operand emitterValueSuggest (emitterCtx* ctx, irBlock** block, const ast* Node, const operand* request) {
    return emitterValueImpl(ctx, block, Node, requestAny, request);
}

static operand emitterValueImpl (emitterCtx* ctx, irBlock** block, const ast* Node,
                                 emitterRequest request, const operand* suggestion) {
    operand Value;

    /*Calculate the value*/

    if (Node->tag == astBOP) {
        if (   Node->o == opDivide || Node->o == opDivideAssign
            || Node->o == opModulo || Node->o == opModuloAssign)
            Value = emitterDivisionBOP(ctx, block, Node);

        else if (   Node->o == opShl || Node->o == opShlAssign
                 || Node->o == opShr || Node->o == opShrAssign)
            Value = emitterShiftBOP(ctx, block, Node);

        else if (opIsAssignment(Node->o))
            Value = emitterAssignmentBOP(ctx, block, Node);

        else if (opIsLogical(Node->o))
            Value = emitterLogicalBOP(ctx, block, Node);

        else
            Value = emitterBOP(ctx, block, Node);

    } else if (Node->tag == astUOP)
        Value = emitterUOP(ctx, block, Node);

    else if (Node->tag == astTOP)
        Value = emitterTOP(ctx, block, Node, suggestion);

    else if (Node->tag == astIndex)
        Value = emitterIndex(ctx, block, Node);

    else if (Node->tag == astCall)
        Value = emitterCall(ctx, block, Node);

    else if (Node->tag == astCast)
        Value = emitterCast(ctx, block, Node);

    else if (Node->tag == astSizeof)
        Value = emitterSizeof(ctx, block, Node);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            Value = emitterSymbol(ctx, block, Node);

        else if (Node->litTag == literalCompound)
            Value = emitterCompoundLiteral(ctx, block, Node);

        else
            Value = emitterLiteral(ctx, block, Node);

    } else {
        debugErrorUnhandled("emitterValueImpl", "AST tag", astTagGetStr(Node->tag));
        Value = operandCreate(operandUndefined);
        request = requestAny;
    }

    /*Put it where requested*/

    /*Allow an array to decay to a pointer unless being used as an array*/
    if (Value.array && request != requestArray) {
        operand address = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->ir, *block, address, Value);
        operandFree(Value);
        Value = address;
    }

    operand Dest;

    if (suggestion) {
        if (!operandIsEqual(Value, *suggestion)) {
            asmMove(ctx->ir, *block, *suggestion, Value);
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
            operand base = emitterGetInReg(ctx, *block, Value, ctx->arch->wordsize);
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
            asmMove(ctx->ir, *block, Dest, Value);
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
            asmEvalAddress(ctx->ir, *block, base, Value);
            operandFree(Value);
            Dest = operandCreateMem(base.base, 0, typeGetSize(ctx->arch, Node->dt));
        }

    /*Compare against zero to get flags*/
    } else if (request == requestFlags) {
        if (Value.tag != operandFlags) {
            asmCompare(ctx->ir, *block, Value, operandCreateLiteral(0));
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

            asmPush(ctx->ir, *block, Value);
            operandFree(Value);

        } else
            Dest = Value;

    }

    return Dest;
}

static operand emitterBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("BOP");

    operand L, R, Value;

    /* '.' */
    if (Node->o == opMember) {
        Value = L = emitterValue(ctx, block, Node->l, requestMem);
        Value.offset += Node->symbol->offset;
        Value.size = typeGetSize(ctx->arch, Node->symbol->dt);

    /* '->' */
    } else if (Node->o == opMemberDeref) {
        L = emitterValue(ctx, block, Node->l, requestReg);

        Value = operandCreateMem(L.base,
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch, Node->dt));

    /*Comparison operator*/
    } else if (opIsEquality(Node->o) || opIsOrdinal(Node->o)) {
        L = emitterValue(ctx, block, Node->l, requestRegOrMem);
        R = emitterValue(ctx, block, Node->r, requestValue);

        Value = operandCreateFlags(conditionNegate(conditionFromOp(Node->o)));
        asmCompare(ctx->ir, *block, L, R);
        operandFree(L);
        operandFree(R);

    /* ',' */
    } else if (Node->o == opComma) {
        operandFree(emitterValue(ctx, block, Node->l, requestAny));
        Value = R = emitterValue(ctx, block, Node->r, requestAny);

    /*Numeric operator*/
    } else {
        Value = L = emitterValue(ctx, block, Node->l, requestReg);
        R = emitterValue(ctx, block, Node->r, requestValue);

        boperation bop = Node->o == opAdd ? bopAdd :
                         Node->o == opSubtract ? bopSub :
                         Node->o == opMultiply ? bopMul :
                         Node->o == opBitwiseAnd ? bopBitAnd :
                         Node->o == opBitwiseOr ? bopBitOr :
                         Node->o == opBitwiseXor ? bopBitXor : bopUndefined;

        if (bop)
            asmBOP(ctx->ir, *block, bop, L, R);

        else
            debugErrorUnhandled("emitterBOP", "operator", opTagGetStr(Node->o));

        operandFree(R);
    }

    debugLeave();

    return Value;
}

static operand emitterDivisionBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("DivisionBOP");

    /*x86 is really non-orthogonal for division. EDX:EAX is treated as the LHS
      and then EDX stores the remainder and EAX the quotient*/

    bool isAssign = Node->o == opDivideAssign || Node->o == opModuloAssign,
         isModulo = Node->o == opModulo || Node->o == opModuloAssign;

    int raxOldSize, rdxOldSize;

    /*RAX: take, but save if necessary*/
    operand RAX = emitterTakeReg(ctx, *block, regRAX, &raxOldSize, typeGetSize(ctx->arch, Node->l->dt));

    /*RDX: take, (save), clear*/
    operand RDX = emitterTakeReg(ctx, *block, regRDX, &rdxOldSize, ctx->arch->wordsize);
    asmMove(ctx->ir, *block, RDX, operandCreateLiteral(0));

    /*RHS*/
    operand Value, L, R = emitterValue(ctx, block, Node->r, requestRegOrMem);

    /*LHS*/

    if (isAssign) {
        L = emitterValue(ctx, block, Node->l, requestMem);
        asmMove(ctx->ir, *block, RAX, L);

    } else
        L = emitterValueSuggest(ctx, block, Node->l, &RAX);

    /*The calculation*/
    asmDivision(ctx->ir, *block, R);
    operandFree(R);

    /*If the result reg was used before, move it to a new reg*/
    if ((isModulo ? rdxOldSize : raxOldSize) != 0) {
        Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
        asmMove(ctx->ir, *block, Value, isModulo ? RDX : RAX);

        /*Restore regs*/
        emitterGiveBackReg(ctx, *block, regRDX, rdxOldSize);
        emitterGiveBackReg(ctx, *block, regRAX, raxOldSize);

    } else {
        if (isModulo) {
            Value = RDX;
            emitterGiveBackReg(ctx, *block, regRAX, raxOldSize);

        } else {
            Value = RAX;
            emitterGiveBackReg(ctx, *block, regRDX, rdxOldSize);
        }
    }

    /*If an assignment, also move the result into memory*/
    if (isAssign) {
        asmMove(ctx->ir, *block, L, Value);
        operandFree(L);
    }

    debugLeave();

    return Value;
}

static operand emitterShiftBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("ShiftBOP");

    operand R;
    int rcxOldSize;

    /*Is the RHS an immediate?*/
    bool immediate =    Node->r->tag == astLiteral
                     && (   Node->r->litTag == literalInt || Node->r->litTag == literalChar
                         || Node->r->litTag == literalBool);

    /*Then use it directly*/
    if (immediate)
        R = emitterLiteral(ctx, block, Node->r);

    else {
        /*Shifting too is non-orthogonal. RHS goes in RCX, CL used as the operand.
          If RHS >= 2^8 or < 0, too bad.*/

        R = emitterTakeReg(ctx, *block, regRCX, &rcxOldSize, typeGetSize(ctx->arch, Node->l->dt));
        emitterValueSuggest(ctx, block, Node->r, &R);

        /*Only CL*/
        R.base->allocatedAs = 1;
    }

    /*Assignment op? Then get the LHS as an lvalue, otherwise in a register*/
    emitterRequest Lrequest =   Node->o == opShlAssign || Node->o == opShrAssign
                              ? requestMem : requestReg;
    operand L = emitterValue(ctx, block, Node->l, Lrequest);

    /*Shift*/
    boperation op = Node->o == opShl || Node->o == opShlAssign ? bopShL : bopShR;
    asmBOP(ctx->ir, *block, op, L, R);

    /*Give back RCX*/
    if (!immediate)
        emitterGiveBackReg(ctx, *block, regRCX, rcxOldSize);

    debugLeave();

    return L;
}

static operand emitterAssignmentBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("AssignnentBOP");

    /*Keep the left in memory so that the lvalue gets modified*/
    operand Value, R = emitterValue(ctx, block, Node->r, requestValue),
                   L = emitterValue(ctx, block, Node->l, requestMem);

    boperation bop = Node->o == opAddAssign ? bopAdd :
                     Node->o == opSubtractAssign ? bopSub :
                     Node->o == opMultiplyAssign ? bopMul :
                     Node->o == opBitwiseAndAssign ? bopBitAnd :
                     Node->o == opBitwiseOrAssign ? bopBitOr :
                     Node->o == opBitwiseXorAssign ? bopBitXor : bopUndefined;

    if (Node->o == opAssign) {
        Value = R;
        asmMove(ctx->ir, *block, L, R);
        operandFree(L);

    } else if (bop != bopUndefined) {
        Value = L;
        asmBOP(ctx->ir, *block, bop, L, R);
        operandFree(R);

    } else
        debugErrorUnhandled("emitterAssignmentBOP", "operator", opTagGetStr(Node->o));

    debugLeave();

    return Value;
}

static operand emitterLogicalBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    /*Label to jump to if circuit gets shorted*/
    irBlock* continuation = irBlockCreate(ctx->ir, ctx->curFn);

    operand Value;

    /*Move the default into Value, possibly jump to the continuation,
      return the condition as flags in R*/
    operand R = emitterLogicalBOPImpl(ctx, block, Node, continuation, &Value);

    if (Node->o == opLogicalAnd)
        R.condition = conditionNegate(R.condition);

    /*Move final value*/
    asmConditionalMove(ctx->ir, *block, R, Value, operandCreateLiteral(Node->o == opLogicalOr ? 0 : 1));

    /*Continue*/
    irJump(*block, continuation);
    *block = continuation;

    return Value;
}

static operand emitterLogicalBOPImpl (emitterCtx* ctx, irBlock** block, const ast* Node, irBlock* shortcont, operand* Value) {
    debugEnter("LogicalBOP");

    /*The job of this function is:
        - Move the default into Value (possibly passing the buck recursively)
        - Branch to the short block depending on LHS
        - Otherwise calculate the RHS and return flags*/

    /*Value is where the default goes, L and R are the conditions*/

    operand L;

    /*If LHS is the same op, then the short value is the same
       => use the given short block, their register*/
    if (Node->l->tag == astBOP && Node->l->o == Node->o)
        L = emitterLogicalBOPImpl(ctx, block, Node->l, shortcont, Value);

    else {
        /*Left*/
        L = emitterValue(ctx, block, Node->l, requestFlags);

        /*Set up the short circuit value*/
        *Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));
        asmMove(ctx->ir, *block, *Value, operandCreateLiteral(Node->o == opLogicalAnd ? 0 : 1));
    }

    /*Branch on the LHS*/

    irBlock* unshort = irBlockCreate(ctx->ir, ctx->curFn);

    if (Node->o == opLogicalOr)
        irBranch(*block, L, shortcont, unshort);

    else
        irBranch(*block, L, unshort, shortcont);

    *block = unshort;

    /*Move the RHS into the flags*/

    operand R = emitterValue(ctx, &unshort, Node->r, requestFlags);

    return R;
}

static operand emitterUOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("UOP");

    operand R, Value;

    /*Increment/decrement ops*/
    if (   Node->o == opPostIncrement || Node->o == opPostDecrement
        || Node->o == opPreIncrement || Node->o == opPreDecrement) {
        R = emitterValue(ctx, block, Node->r, requestMem);

        bool post = Node->o == opPostIncrement || Node->o == opPostDecrement;

        /*Post ops: save a copy before inc/dec*/
        if (post) {
            Value = operandCreateReg(regAlloc(operandGetSize(ctx->arch, R)));
            asmMove(ctx->ir, *block, Value, R);

        } else
            Value = R;

        uoperation op =   Node->o == opPostIncrement || Node->o == opPreIncrement
                        ? uopInc : uopDec;

        asmUOP(ctx->ir, *block, op, R);

        if (post)
            operandFree(R);

    /*Numerical and logical ops*/
    } else if (   Node->o == opNegate || Node->o == opUnaryPlus
               || Node->o == opBitwiseNot || Node->o == opLogicalNot) {
        R = emitterValue(ctx, block, Node->r, requestReg);

        if (Node->o == opLogicalNot) {
            asmCompare(ctx->ir, *block, R, operandCreateLiteral(0));
            Value = operandCreateFlags(conditionNotEqual);
            operandFree(R);

        } else if (Node->o == opUnaryPlus) {
            Value = R;

        } else {
            asmUOP(ctx->ir, *block, Node->o == opNegate ? uopNeg : uopBitwiseNot, R);
            Value = R;
        }

    /*Deref*/
    } else if (Node->o == opDeref) {
        operand Ptr = emitterValue(ctx, block, Node->r, requestReg);
        Value = operandCreateMem(Ptr.base, 0, typeGetSize(ctx->arch, Node->dt));

    /*Address of*/
    } else if (Node->o == opAddressOf) {
        R = emitterValue(ctx, block, Node->r, requestMem);

        Value = operandCreateReg(regAlloc(ctx->arch->wordsize));

        asmEvalAddress(ctx->ir, *block, Value, R);
        operandFree(R);

    } else {
        debugErrorUnhandled("emitterUOP", "operator", opTagGetStr(Node->o));
        Value = operandCreateInvalid();
    }

    debugLeave();

    return Value;
}

static operand emitterTOP (emitterCtx* ctx, irBlock** block, const ast* Node, const operand* suggestion) {
    debugEnter("TOP");

    irBlock *ifTrue = irBlockCreate(ctx->ir, ctx->curFn),
            *ifFalse = irBlockCreate(ctx->ir, ctx->curFn),
            *continuation = irBlockCreate(ctx->ir, ctx->curFn);

    /*Condition, branch*/
    emitterBranchOnValue(ctx, *block, Node->firstChild, ifTrue, ifFalse);

    /*Ask for LHS to go in a reg, or the suggestion. This becomes our return*/
    operand Value = emitterValueImpl(ctx, &ifTrue, Node->l, requestReg, suggestion);
    irJump(ifTrue, continuation);

    /*Move RHS into our reg*/
    emitterValueSuggest(ctx, &ifFalse, Node->r, &Value);
    irJump(ifFalse, continuation);

    *block = continuation;

    debugLeave();

    return Value;
}

static operand emitterIndex (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("Index");

    operand L, R, Value;

    int size = typeGetSize(ctx->arch, Node->dt);

    /*Is it an array? Are we directly offsetting the address*/
    if (typeIsArray(Node->l->dt)) {
        L = emitterValue(ctx, block, Node->l, requestArray);
        R = emitterValue(ctx, block, Node->r, requestValue);

        /*Is the RHS just a constant? Add it to the offset*/
        if (R.tag == operandLiteral) {
            Value = L;
            Value.offset += size*R.literal;

        /*LHS has an index but factor matches? Add RHS to the index*/
        } else if (L.index && L.factor == size) {
            asmBOP(ctx->ir, *block, bopAdd, operandCreateReg(L.index), R);
            operandFree(R);
            Value = L;

        } else {
            R = emitterGetInReg(ctx, *block, R, typeGetSize(ctx->arch, Node->r->dt));

            /*If L doesn't have an index, we can use it directly*/
            if (!L.index) {
                Value = L;
                Value.tag = operandMem;

            /*Evaluate the address of L, use the result as base of new operand*/
            } else {
                Value = operandCreateMem(regAlloc(ctx->arch->wordsize), 0, size);
                asmEvalAddress(ctx->ir, *block, operandCreateReg(Value.base), L);
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
                asmBOP(ctx->ir, *block, bopMul, R, operandCreateLiteral(multiplier));
        }

        Value.size = size;
        Value.array = false;

    /*Is it instead a pointer? Get value and offset*/
    } else /*if (typeIsPtr(Node->l->dt)*/ {
        /* index = R*sizeof(*L) */
        R = emitterValue(ctx, block, Node->r, requestReg);
        asmBOP(ctx->ir, *block, bopMul, R, operandCreateLiteral(size));

        L = emitterValue(ctx, block, Node->l, requestValue);

        asmBOP(ctx->ir, *block, bopAdd, R, L);
        operandFree(L);

        Value = operandCreateMem(R.base, 0, size);
    }

    debugLeave();

    return Value;
}

static operand emitterCall (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("Call");

    operand Value;

    /*Caller save registers: only if in use*/
    for (int i = 0; i < ctx->arch->scratchRegs.length; i++) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->scratchRegs, i);

        if (regIsUsed(r))
            asmSaveReg(ctx->ir, *block, r);
    }

    int argSize = 0;

    /*If larger than a word, the return will be passed in a temporary position
      (stack) allocated by the caller.*/
    bool retInTemp = typeGetSize(ctx->arch, Node->dt) > ctx->arch->wordsize;
    int tempWords = 0;

    if (retInTemp) {
        /*Allocate the temporary space (rounded up to the nearest word)*/
        tempWords = (typeGetSize(ctx->arch, Node->dt)-1)/ctx->arch->wordsize + 1;
        asmPushN(ctx->ir, *block, tempWords);
    }

    /*Push the args on backwards (cdecl)*/
    for (ast* Current = Node->lastChild;
         Current;
         Current = Current->prevSibling) {
        operand Arg = emitterValue(ctx, block, Current, requestStack);
        argSize += Arg.size;
    }

    /*Pass on the reference to the temporary return space
      Last, so that a varargs fn can still locate it*/
    if (retInTemp) {
        operand intermediate = operandCreateReg(regAlloc(ctx->arch->wordsize));
        asmEvalAddress(ctx->ir, *block, intermediate, operandCreateMem(&regs[regRSP], argSize, ctx->arch->wordsize));
        asmPush(ctx->ir, *block, intermediate);
        operandFree(intermediate);
    }

    /*Call the function*/

    irBlock* continuation = irBlockCreate(ctx->ir, ctx->curFn);

    if (typeIsFunction(Node->l->symbol->dt))
        irCall(*block, Node->l->symbol, continuation);

    else {
        operand fn = emitterValue(ctx, block, Node->l, requestAny);
        irCallIndirect(*block, fn, continuation);
        operandFree(fn);
    }

    *block = continuation;

    if (!typeIsVoid(Node->dt)) {
        int size = retInTemp ? ctx->arch->wordsize : typeGetSize(ctx->arch, Node->dt);

        /*If RAX is already in use (currently backed up to the stack), relocate the
          return value to another free reg before RAX's value is restored.*/
        if (regIsUsed(regRAX)) {
            Value = operandCreateReg(regAlloc(size));
            int tmp = regs[regRAX].allocatedAs;
            regs[regRAX].allocatedAs = size;
            asmMove(ctx->ir, *block, Value, operandCreateReg(&regs[regRAX]));
            regs[regRAX].allocatedAs = tmp;

        } else
            Value = operandCreateReg(regRequest(regRAX, size));

        /*The temporary's pointer is returned to us*/
        if (retInTemp)
            Value = operandCreateMem(Value.base, 0, typeGetSize(ctx->arch, Node->dt));

    } else
        Value = operandCreateVoid();

    /*Pop the args (and temporary return space & reference)*/
    asmPopN(ctx->ir, *block, argSize/ctx->arch->wordsize + tempWords + (retInTemp ? 1 : 0));

    /*Restore the saved registers (backwards as stacks are LIFO)*/
    for (int i = ctx->arch->scratchRegs.length-1; i >= 0 ; i--) {
        regIndex r = (regIndex) vectorGet(&ctx->arch->scratchRegs, i);

        if (regIsUsed(r) && regGet(r) != Value.base)
            asmRestoreReg(ctx->ir, *block, r);
    }

    debugLeave();

    return Value;
}

static operand emitterCast (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("Cast");

    operand R = emitterValue(ctx, block, Node->r, requestValue);

    /*Widen or narrow, if needed*/

    int from = typeGetSize(ctx->arch, Node->r->dt),
        to = typeGetSize(ctx->arch, Node->dt);

    if (from != to && to != 0)
        R = (from < to ? emitterWiden : emitterNarrow)(ctx, *block, R, to);

    debugLeave();

    return R;
}

static operand emitterSizeof (emitterCtx* ctx, irBlock** block, const ast* Node) {
    (void) block;

    debugEnter("Sizeof");

    const type* DT = Node->r->dt;
    operand Value = operandCreateLiteral(typeGetSize(ctx->arch, DT));

    debugLeave();

    return Value;
}

static operand emitterSymbol (emitterCtx* ctx, irBlock** block, const ast* Node) {
    (void) block;

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

static operand emitterLiteral (emitterCtx* ctx, irBlock** block, const ast* Node) {
    (void) ctx;

    debugEnter("Literal");

    operand Value;

    if (Node->litTag == literalInt)
        Value = operandCreateLiteral(*(int*) Node->literal);

    else if (Node->litTag == literalChar)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else if (Node->litTag == literalBool)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else if (Node->litTag == literalStr)
        Value = irStringConstant(ctx->ir, (char*) Node->literal);

    else {
        debugErrorUnhandled("emitterLiteral", "literal tag", literalTagGetStr(Node->litTag));
        Value = operandCreateInvalid();
    }

    debugLeave();

    return Value;
}

static operand emitterCompoundLiteral (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("CompoundLiteral");

    operand Value = emitterSymbol(ctx, block, Node);
    emitterCompoundInit(ctx, block, Node, Value);

    debugLeave();

    return Value;
}

void emitterCompoundInit (emitterCtx* ctx, irBlock** block, const ast* Node, operand base) {
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

            emitterElementInit(ctx, block, value, L);
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
            emitterElementInit(ctx, block, value, L);
            L.offset += elementSize;
        }

    /*Scalar*/
    } else
        emitterValueSuggest(ctx, block, Node->firstChild, &base);
}

static void emitterElementInit (emitterCtx* ctx, irBlock** block, const ast* Node, operand L) {
    /*Skipped init*/
    if (Node->tag == astEmpty)
        ;

    /*Recursive initialization*/
    else if (Node->tag == astLiteral && Node->litTag == literalInit)
        emitterCompoundInit(ctx, block, Node, L);

    /*Regular value*/
    else
        emitterValueSuggest(ctx, block, Node, &L);
}
