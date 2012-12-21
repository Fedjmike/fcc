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

/*Emitter context*/

static void emitterModule (emitterCtx* ctx, ast* Tree);
static void emitterStruct (emitterCtx* ctx, sym* Symbol);
static void emitterEnum (emitterCtx* ctx, sym* Symbol);
static void emitterFunction (emitterCtx* ctx, ast* Node);
static void emitterCode (emitterCtx* ctx, ast* Node);
static void emitterLine (emitterCtx* ctx, ast* Node);

static void emitterBranch (emitterCtx* ctx, ast* Node);
static void emitterLoop (emitterCtx* ctx, ast* Node);
static void emitterIter (emitterCtx* ctx, ast* Node);
static void emitterVar (emitterCtx* ctx, ast* Node);
static void emitterArrayLiteral (emitterCtx* ctx, ast* Node, sym* Symbol);

static emitterCtx* emitterInit (FILE* File) {
    emitterCtx* ctx = malloc(sizeof(emitterCtx));
    ctx->Asm = asmInit(File);
    ctx->labelReturnTo = operandCreate(operandUndefined);
    ctx->labelBreakTo = operandCreate(operandUndefined);
    return ctx;
}

static void emitterEnd (emitterCtx* ctx) {
    asmEnd(ctx->Asm);
    free(ctx);
}

void emitter (ast* Tree, FILE* File) {
    emitterCtx* ctx = emitterInit(File);
    asmFilePrologue(ctx->Asm);

    labelFreeAll();

    emitterModule(ctx, Tree);

    asmFileEpilogue(ctx->Asm);
    emitterEnd(ctx);
}

void emitterModule (emitterCtx* ctx, ast* Tree) {
    debugEnter("Module");

    /*Resolve struct info*/
    for (sym* Current = Tree->symbol->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->class == symStruct)
            emitterStruct(ctx, Current);

        else if (Current->class == symEnum)
            emitterEnum(ctx, Current);

        else if (Current->class == symType ||
                 Current->class == symFunction)
            ; //Nothing to do

        else
            debugErrorUnhandled("emitterModule", "symbol", symClassGetStr(Current->class));
    }

    /*Functions*/
    for (ast* Current = Tree->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->class == astFunction)
            emitterFunction(ctx, Current);

        else
            debugErrorUnhandled("emitterModule", "AST class", astClassGetStr(Current->class));
    }

    debugLeave();
}

void emitterStruct (emitterCtx* ctx, sym* Symbol) {
    debugEnter("Struct");

    for (sym* Current = Symbol->firstChild;
         Current;
         Current = Current->nextSibling) {

        Current->offset = Symbol->size;
        /*Add the size of this field, rounded up to the nearest word boundary*/
        Symbol->size += ((typeGetSize(Current->dt) - 1)/8)*8 + 8;
        reportSymbol(Current);
    }

    reportSymbol(Symbol);

    debugLeave();
}

void emitterEnum (emitterCtx* ctx, sym* Symbol) {
    debugEnter("Enum");

    for (sym* Current = Symbol->firstChild;
         Current;
         Current = Current->nextSibling) {

    }

    debugLeave();
}

void emitterFunction (emitterCtx* ctx, ast* Node) {
    debugEnter("Function");

    Node->symbol->label = labelNamed(Node->symbol->ident);

    int lastOffset = 16;
    sym* Symbol;

    /*Asign offsets to all the parameters*/
    for (Symbol = Node->symbol->firstChild;
         Symbol && Symbol->class == symParam;
         Symbol = Symbol->nextSibling) {
        Symbol->offset = lastOffset;
        lastOffset += typeGetSize(Symbol->dt);

        reportSymbol(Symbol);
    }

    lastOffset = 0;

    /*And then the local variables which follow directly*/
    for (;
         Symbol;
         Symbol = Symbol->nextSibling) {
        lastOffset -= typeGetSize(Symbol->dt);
        Symbol->offset = lastOffset;

        reportSymbol(Symbol);
    }

    /*Label to jump to from returns*/
    operand EndLabel = ctx->labelReturnTo = labelCreate(labelReturn);

    asmComment(ctx->Asm, "");
    asmFnPrologue(ctx->Asm,
                  labelGet(Node->symbol->label),
                  Node->symbol->lastChild && Node->symbol->lastChild->class != symParam
                        ? -Node->symbol->lastChild->offset : 0);
    emitterCode(ctx, Node->r);
    asmFnEpilogue(ctx->Asm, labelGet(EndLabel));

    debugLeave();
}

void emitterCode (emitterCtx* ctx, ast* Node) {
    asmEnter(ctx->Asm);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        emitterLine(ctx, Current);
    }

    asmLeave(ctx->Asm);
}

void emitterLine (emitterCtx* ctx, ast* Node) {
    debugEnter("Line");

    asmComment(ctx->Asm, "");

    if (Node->class == astBranch)
        emitterBranch(ctx, Node);

    else if (Node->class == astLoop)
        emitterLoop(ctx, Node);

    else if (Node->class == astIter)
        emitterIter(ctx, Node);

    else if (Node->class == astReturn) {
        emitterValue(ctx, Node->r, operandCreateReg(regRAX));
        asmJump(ctx->Asm, ctx->labelReturnTo);

    } else if (Node->class == astVar)
        emitterVar(ctx, Node);

    else if (astIsValueClass(Node->class))
        operandFree(emitterValue(ctx, Node, operandCreate(operandUndefined)));

    else
        debugErrorUnhandled("emitterLine", "AST class", astClassGetStr(Node->class));

    debugLeave();
}

void emitterBranch (emitterCtx* ctx, ast* Node) {
    debugEnter("Branch");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);

    /*Compute the condition, requesting it be placed in the flags*/
    asmBranch(ctx->Asm,
              emitterValue(ctx,
                           Node->firstChild,
                           operandCreateFlags(conditionUndefined)),
              ElseLabel);

    emitterCode(ctx, Node->l);

    asmComment(ctx->Asm, "");
    asmJump(ctx->Asm, EndLabel);
    asmLabel(ctx->Asm, ElseLabel);

    emitterCode(ctx, Node->r);

    asmLabel(ctx->Asm, EndLabel);

    debugLeave();
}

void emitterLoop (emitterCtx* ctx, ast* Node) {
    debugEnter("Loop");

    operand LoopLabel = labelCreate(labelUndefined); /*The place to return to loop again (after confirming condition)*/
    operand OldBreakTo = ctx->labelBreakTo; /*Push old break label before erasing*/
    operand EndLabel = ctx->labelBreakTo = labelCreate(labelUndefined); /*To exit the loop (or break, anywhere)*/

    /*Work out which order the condition and code came in
      => whether this is a while or a do while*/
    bool isDo = Node->l->class == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    if (!isDo)
        asmBranch(ctx->Asm,
                  emitterValue(ctx, cond, operandCreateFlags(conditionUndefined)),
                  EndLabel);

    asmLabel(ctx->Asm, LoopLabel);
    emitterCode(ctx, code);

    asmComment(ctx->Asm, "");

    asmBranch(ctx->Asm,
              emitterValue(ctx, cond, operandCreateFlags(conditionUndefined)),
              EndLabel);

    asmJump(ctx->Asm, LoopLabel);
    asmLabel(ctx->Asm, EndLabel);

    ctx->labelBreakTo = OldBreakTo;

    debugLeave();
}

void emitterIter (emitterCtx* ctx, ast* Node) {
    debugEnter("Iter");

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    operand LoopLabel = labelCreate(labelUndefined);
    operand OldBreakTo = ctx->labelBreakTo;
    operand EndLabel = ctx->labelBreakTo = labelCreate(labelUndefined);

    /*Initialize stuff*/

    if (init->class == astVar)
        emitterVar(ctx, init);

    else if (astIsValueClass(init->class))
        operandFree(emitterValue(ctx, init, operandCreate(operandUndefined)));

    else if (init->class != astEmpty)
        debugErrorUnhandled("emitterIter", "AST class", astClassGetStr(init->class));

    asmComment(ctx->Asm, "");

    /*Check condition*/

    asmLabel(ctx->Asm, LoopLabel);

    if (cond->class != astEmpty) {
        operand Condition = emitterValue(ctx, cond, operandCreateFlags(conditionUndefined));
        asmBranch(ctx->Asm, Condition, EndLabel);
    }

    /*Do block*/

    emitterCode(ctx, Node->l);

    /*Iterate*/

    asmComment(ctx->Asm, "");

    if (iter->class != astEmpty)
        operandFree(emitterValue(ctx, iter, operandCreate(operandUndefined)));

    /*loopen Sie*/

    asmComment(ctx->Asm, "");
    asmJump(ctx->Asm, LoopLabel);
    asmLabel(ctx->Asm, EndLabel);

    ctx->labelBreakTo = OldBreakTo;

    debugLeave();
}

void emitterVar (emitterCtx* ctx, ast* Node) {
    debugEnter("Var");

    /*Is there an initial value assigned?*/
    if (Node->r) {
        if (Node->r->class == astLiteral && Node->r->litClass == literalArray)
            emitterArrayLiteral(ctx, Node->r, Node->symbol);

        else if (Node->symbol->storage == storageAuto) {
            asmEnter(ctx->Asm);
            operand L = operandCreateMem(regRBP, Node->symbol->offset, Node->symbol->dt.basic->size);
            operand R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
            asmLeave(ctx->Asm);
            asmMove(ctx->Asm, L, R);
            operandFree(R);

        } else
            debugErrorUnhandled("emitterVar", "storage class", storageClassGetStr(Node->symbol->storage));
    }

    if (Node->l)
        emitterVar(ctx, Node->l);

    debugLeave();
}

void emitterArrayLiteral (emitterCtx* ctx, ast* Node, sym* Symbol) {
    debugEnter("ArrayLiteral");

    int n = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling, n++) {
        asmEnter(ctx->Asm);
        operand L = operandCreateMem(regRBP,
                                     Symbol->offset + Symbol->dt.basic->size*n,
                                     Symbol->dt.basic->size);
        operand R = emitterValue(ctx, Current, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);
        asmMove(ctx->Asm, L, R);
        operandFree(R);
    }

    debugLeave();
}
