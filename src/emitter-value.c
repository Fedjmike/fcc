#include "../inc/emitter-value.h"

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

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

static operand emitterBOP (emitterCtx* ctx, const ast* Node);
static operand emitterLogicalBOP (emitterCtx* ctx, const ast* Node);
static operand emitterAssignmentBOP (emitterCtx* ctx, const ast* Node);

static operand emitterUOP (emitterCtx* ctx, const ast* Node);
static operand emitterTOP (emitterCtx* ctx, const ast* Node);
static operand emitterIndex (emitterCtx* ctx, const ast* Node);
static operand emitterCall (emitterCtx* ctx, const ast* Node);
static operand emitterSizeof (emitterCtx* ctx, const ast* Node);
static operand emitterSymbol (emitterCtx* ctx, const ast* Node);
static operand emitterLiteral (emitterCtx* ctx, const ast* Node);

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
                asmBOP(ctx->Asm, bopCmp, Value, operandCreateLiteral(0));
                operandFree(Value);

                Dest = operandCreateFlags(conditionEqual);

            } else if (Dest.tag == operandReg) {
                /*If a specific register wasn't requested, allocate one*/
                if (Dest.reg == regUndefined)
                    Dest.reg = regAlloc(typeGetSize(ctx->arch, Node->dt));

                asmMove(ctx->Asm, Dest, Value);
                operandFree(Value);

            } else if (Dest.tag == operandMem) {
                if (Value.tag == operandMemRef) {
                    Dest = Value;
                    Dest.tag = operandMem;

                } else
                    printf("emitterValue(): unable to convert non lvalue operand tag, %s.\n", operandTagGetStr(Value.tag));

            } else if (Dest.tag == operandStack) {
                /*Larger than a word?*/
                if (Value.tag == operandMem && Value.size > ctx->arch->wordsize) {
                    int total = Value.size;

                    /*Then push on *backwards* in word chunks.
                      Start at the highest address*/
                    Value.offset += total - ctx->arch->wordsize;
                    Value.size = ctx->arch->wordsize;

                    for (int i = 0; i < total; i += ctx->arch->wordsize) {
                        asmPush(ctx->Asm, Value);
                        Value.offset -= ctx->arch->wordsize;
                    }

                    Dest.size = total;

                } else {
                    asmPush(ctx->Asm, Value);
                    operandFree(Value);
                    Dest.size = ctx->arch->wordsize;
                }

            } else if (Value.tag == operandUndefined)
                printf("emitterValue(): expected value, void given.\n");

            else {
                debugErrorUnhandled("emitterValue", "operand tag", operandTagGetStr(Value.tag));
                debugErrorUnhandled("emitterValue", "operand tag", operandTagGetStr(Dest.tag));
            }

        } else if (   Dest.tag == operandReg
                   && Dest.reg != Value.reg
                   && Dest.reg != regUndefined) {
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

        Value = operandCreateMem(L.reg,
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
        asmBOP(ctx->Asm, bopCmp, L, R);
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

        if (!strcmp(Node->o, "+"))
            asmBOP(ctx->Asm, bopAdd, L, R);

        else if (!strcmp(Node->o, "-"))
            asmBOP(ctx->Asm, bopSub, L, R);

        else if (!strcmp(Node->o, "*"))
            asmBOP(ctx->Asm, bopMul, L, R);

        else if (!strcmp(Node->o, "&"))
            asmBOP(ctx->Asm, bopBitAnd, L, R);

        else if (!strcmp(Node->o, "|"))
            asmBOP(ctx->Asm, bopBitOr, L, R);

        else if (!strcmp(Node->o, "^"))
            asmBOP(ctx->Asm, bopBitXor, L, R);

        else if (!strcmp(Node->o, ">>"))
            asmBOP(ctx->Asm, bopShR, L, R);

        else if (!strcmp(Node->o, "<<"))
            asmBOP(ctx->Asm, bopShL, L, R);

        else
            debugErrorUnhandled("emitterBOP", "operator", Node->o);

        operandFree(R);
    }

    debugLeave();

    return Value;
}

static operand emitterLogicalBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("LogicalBOP");

    operand L, R, Value = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->dt)));

    /*Set up the short circuit value*/
    asmMove(ctx->Asm, Value, operandCreateLiteral(
                !strcmp(Node->o, "&&") ? 0 : 1));

    /*Label to jump to if circuit gets shorted*/
    operand ShortLabel = labelCreate(labelUndefined);

    /*Left*/
    asmEnter(ctx->Asm);
    L = emitterValue(ctx, Node->l, operandCreate(operandFlags));
    asmLeave(ctx->Asm);

    /*Check initial condition*/

    if (!strcmp(Node->o, "||"))
        L.condition = conditionNegate(L.condition);

    asmBranch(ctx->Asm, L, ShortLabel);

    /*Right*/
    asmEnter(ctx->Asm);
    R = emitterValue(ctx, Node->r, operandCreate(operandFlags));
    asmLeave(ctx->Asm);

    if (!strcmp(Node->o, "&&"))
        R.condition = conditionNegate(R.condition);

    asmConditionalMove(ctx->Asm, R, Value, operandCreateLiteral(1));
    asmLabel(ctx->Asm, ShortLabel);

    debugLeave();

    return Value;
}

static operand emitterAssignmentBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("AssignnentBOP");

    asmEnter(ctx->Asm);
    operand Value, R, L = emitterValue(ctx, Node->l, operandCreate(operandMem));
    asmLeave(ctx->Asm);

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

        else if (!strcmp(Node->o, "*="))
            asmBOP(ctx->Asm, bopMul, L, R);

        else if (!strcmp(Node->o, "&="))
            asmBOP(ctx->Asm, bopBitAnd, L, R);

        else if (!strcmp(Node->o, "|="))
            asmBOP(ctx->Asm, bopBitOr, L, R);

        else if (!strcmp(Node->o, "^="))
            asmBOP(ctx->Asm, bopBitXor, L, R);

        else if (!strcmp(Node->o, ">>="))
            asmBOP(ctx->Asm, bopShR, L, R);

        else if (!strcmp(Node->o, "<<="))
            asmBOP(ctx->Asm, bopShL, L, R);

        else
            debugErrorUnhandled("emitterAssignmentBOP", "operator", Node->o);
    }

    Value = R;
    operandFree(L);

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
               || !strcmp(Node->o, "~")) {
        asmEnter(ctx->Asm);
        Value = R = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        asmLeave(ctx->Asm);

        asmUOP(ctx->Asm, !strcmp(Node->o, "-") ? uopNeg : uopBitwiseNot, R);

    } else if (!strcmp(Node->o, "*")) {
        operand Ptr = emitterValue(ctx, Node->r, operandCreateReg(regUndefined));
        Value = operandCreateMem(Ptr.reg, 0, typeGetSize(ctx->arch, Node->dt));

    } else if (!strcmp(Node->o, "&")) {
        asmEnter(ctx->Asm);
        R = emitterValue(ctx, Node->r, operandCreate(operandMem));
        R.size = ctx->arch->wordsize;
        asmLeave(ctx->Asm);

        Value = operandCreateReg(regAlloc(ctx->arch->wordsize));

        asmEvalAddress(ctx->Asm, Value, R);
        //TODO: free R?

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
            Value.index = R.reg;
            Value.factor = typeGetSize(ctx->arch, Node->l->dt->base);

        } else {
            operand index = operandCreateReg(regAlloc(typeGetSize(ctx->arch, Node->r->dt)));
            asmMove(ctx->Asm, index, R);
            operandFree(R);

            Value = L;
            Value.index = index.reg;
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

        Value = operandCreateMem(R.reg, 0, typeGetSize(ctx->arch, Node->dt));
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

    if (!typeIsVoid(Node->dt)) {
        /*If RAX is already in use (currently backed up to the stack), relocate the
          return value to another free reg before RAXs value is restored.*/
        if (regIsUsed(regRAX)) {
            int size = typeGetSize(ctx->arch, Node->dt);
            Value = operandCreateReg(regAlloc(size));
            int tmp = regs[regRAX].allocatedAs;
            regs[regRAX].allocatedAs = size;
            asmMove(ctx->Asm, Value, operandCreateReg(&regs[regRAX]));
            regs[regRAX].allocatedAs = tmp;

        } else
            Value = operandCreateReg(regRequest(regRAX, typeGetSize(ctx->arch, Node->dt)));

    } else
        Value = operandCreateVoid();

    /*Pop the args*/
    asmPopN(ctx->Asm, argSize/ctx->arch->wordsize);

    /*Restore the saved registers (backwards as stacks are LIFO)*/
    for (regIndex r = regR15; r >= regRAX; r--)
        //Attempt to restore all but the one we just allocated for the ret value
        if (regIsUsed(r) && &regs[r] != Value.reg)
            asmPop(ctx->Asm, operandCreateReg(&regs[r]));

    debugLeave();

    return Value;
}

static operand emitterSizeof (emitterCtx* ctx, const ast* Node) {
    (void) ctx;

    debugEnter("Sizeof");

    const type* DT = Node->r->dt;
    operand Value = operandCreateLiteral(typeGetSize(ctx->arch, DT));

    debugLeave();

    return Value;
}

static operand emitterSymbol (emitterCtx* ctx, const ast* Node) {
    (void) ctx;

    debugEnter("Symbol");

    operand Value = operandCreate(operandUndefined);

    if (typeIsFunction(Node->symbol->dt))
        Value = Node->symbol->label;

    else /*var or param*/ {
        if (typeIsArray(Node->symbol->dt))
            Value = operandCreateMemRef(&regs[regRBP],
                                        Node->symbol->offset,
                                        typeGetSize(ctx->arch, Node->symbol->dt->base));

        else
            Value = operandCreateMem(&regs[regRBP],
                                     Node->symbol->offset,
                                     typeGetSize(ctx->arch, Node->symbol->dt));
    }

    debugLeave();

    return Value;
}

static operand emitterLiteral (emitterCtx* ctx, const ast* Node) {
    (void) ctx;

    debugEnter("Literal");

    operand Value;

    if (Node->litTag == literalInt)
        Value = operandCreateLiteral(*(int*) Node->literal);

    else if (Node->litTag == literalBool)
        Value = operandCreateLiteral(*(char*) Node->literal);

    else if (Node->litTag == literalStr) {
        operand ConstLabel = labelCreate(labelROData);
        asmStringConstant(ctx->Asm, ConstLabel, (char*) Node->literal);
        Value = operandCreateLabelOffset(ConstLabel);

    } else {
        debugErrorUnhandled("emitterLiteral", "literal tag", literalTagGetStr(Node->litTag));
        Value = operandCreateInvalid();
    }

    debugLeave();

    return Value;
}
