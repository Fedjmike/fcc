#include "../inc/emitter.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"
#include "../inc/reg.h"

#include "../inc/emitter-value.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

static void emitterModule (emitterCtx* ctx, const ast* Tree);

static void emitterFnImpl (emitterCtx* ctx, const ast* Node);
static void emitterDeclStruct (emitterCtx* ctx, ast* Node);

static void emitterDecl (emitterCtx* ctx, const ast* Node);
static void emitterArrayLiteral (emitterCtx* ctx, const ast* Node, const sym* Symbol);

static void emitterCode (emitterCtx* ctx, const ast* Node);
static void emitterLine (emitterCtx* ctx, const ast* Node);

static void emitterBranch (emitterCtx* ctx, const ast* Node);
static void emitterLoop (emitterCtx* ctx, const ast* Node);
static void emitterIter (emitterCtx* ctx, const ast* Node);

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

void emitter (const ast* Tree, FILE* File) {
    emitterCtx* ctx = emitterInit(File);
    asmFilePrologue(ctx->Asm);

    emitterModule(ctx, Tree);

    labelFreeAll();

    asmFileEpilogue(ctx->Asm);
    emitterEnd(ctx);
}

static void emitterModule (emitterCtx* ctx, const ast* Tree) {
    debugEnter("Module");

    for (ast* Current = Tree->firstChild;
         Current;
         Current = Current->nextSibling) {
        /*!!!
          what?*/
        if (Current->class == astFnImpl)
            emitterFnImpl(ctx, Current);

        else if (Current->class == astDeclStruct)
            emitterDeclStruct(ctx, Current);

        else if (Current->class == astDecl)
            emitterDecl(ctx, Current);

        else
            debugErrorUnhandled("emitterModule", "AST class", astClassGetStr(Current->class));
    }

    debugLeave();
}

static void emitterFnImpl (emitterCtx* ctx, const ast* Node) {
    debugEnter("FnImpl");

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
                  Node->symbol->label,
                  Node->symbol->lastChild && Node->symbol->lastChild->class != symParam
                        ? -Node->symbol->lastChild->offset : 0);
    emitterCode(ctx, Node->r);
    asmFnEpilogue(ctx->Asm, EndLabel);

    debugLeave();
}

static void emitterDeclStruct (emitterCtx* ctx, ast* Node) {
    (void) ctx;

    debugEnter("DeclStruct");

    for (sym* Current = Node->symbol->firstChild;
         Current;
         Current = Current->nextSibling) {

        Current->offset = Node->symbol->size;
        /*Add the size of this field, rounded up to the nearest word boundary*/
        Node->symbol->size += ((typeGetSize(Current->dt) - 1)/8)*8 + 8;
        reportSymbol(Current);
    }

    reportSymbol(Node->symbol);

    debugLeave();
}

static void emitterDecl (emitterCtx* ctx, const ast* Node) {
    (void) ctx;

    debugEnter("Decl");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        /*Initial assignment*/
        if (Current->class == astBOP && !strcmp(Current->o, "=")) {
            /*Array*/
            if (Current->r->class == astLiteral && Current->r->litClass == literalArray)
                emitterArrayLiteral(ctx, Current->r, Current->symbol);

            else if (Current->symbol->storage == storageAuto) {
                asmEnter(ctx->Asm);
                operand L = operandCreateMem(regRBP, Current->symbol->offset, typeGetSize(Current->symbol->dt));
                operand R = emitterValue(ctx, Current->r, operandCreate(operandUndefined));
                asmLeave(ctx->Asm);
                asmMove(ctx->Asm, L, R);
                operandFree(R);

            } else
                debugErrorUnhandled("emitterDecl", "storage class", storageClassGetStr(Node->symbol->storage));
        }
    }

    debugLeave();
}

static void emitterArrayLiteral (emitterCtx* ctx, const ast* Node, const sym* Symbol) {
    debugEnter("ArrayLiteral");

    int n = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling, n++) {
        asmEnter(ctx->Asm);
        operand L = operandCreateMem(regRBP,
                                     Symbol->offset + typeGetSize(Symbol->dt->base)*n,
                                     typeGetSize(Symbol->dt->base));
        operand R = emitterValue(ctx, Current, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);
        asmMove(ctx->Asm, L, R);
        operandFree(R);
    }

    debugLeave();
}

static void emitterCode (emitterCtx* ctx, const ast* Node) {
    asmEnter(ctx->Asm);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        emitterLine(ctx, Current);
    }

    asmLeave(ctx->Asm);
}

static void emitterLine (emitterCtx* ctx, const ast* Node) {
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

    } else if (Node->class == astBreak)
        asmJump(ctx->Asm, ctx->labelBreakTo);

    else if (Node->class == astDecl)
        emitterDecl(ctx, Node);

    else if (astIsValueClass(Node->class))
        operandFree(emitterValue(ctx, Node, operandCreate(operandUndefined)));

    else
        debugErrorUnhandled("emitterLine", "AST class", astClassGetStr(Node->class));

    debugLeave();
}

static void emitterBranch (emitterCtx* ctx, const ast* Node) {
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

    if (Node->r) {
        asmComment(ctx->Asm, "");
        asmJump(ctx->Asm, EndLabel);
        asmLabel(ctx->Asm, ElseLabel);

        emitterCode(ctx, Node->r);

        asmLabel(ctx->Asm, EndLabel);

    } else
        asmLabel(ctx->Asm, ElseLabel);

    debugLeave();
}

static void emitterLoop (emitterCtx* ctx, const ast* Node) {
    debugEnter("Loop");

    /*The place to return to loop again (after confirming condition)*/
    operand LoopLabel = labelCreate(labelUndefined);
    /*Push old break label before erasing*/
    operand OldBreakTo = ctx->labelBreakTo;
    /*To exit the loop (or break, anywhere)*/
    operand EndLabel = ctx->labelBreakTo = labelCreate(labelUndefined);

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

static void emitterIter (emitterCtx* ctx, const ast* Node) {
    debugEnter("Iter");

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    operand LoopLabel = labelCreate(labelUndefined);
    operand OldBreakTo = ctx->labelBreakTo;
    operand EndLabel = ctx->labelBreakTo = labelCreate(labelUndefined);

    /*Initialize stuff*/

    if (init->class == astDecl) {
        emitterDecl(ctx, init);
        asmComment(ctx->Asm, "");

    } else if (astIsValueClass(init->class)) {
        operandFree(emitterValue(ctx, init, operandCreate(operandUndefined)));
        asmComment(ctx->Asm, "");

    } else if (init->class != astEmpty)
        debugErrorUnhandled("emitterIter", "AST class", astClassGetStr(init->class));


    /*Check condition*/

    asmLabel(ctx->Asm, LoopLabel);

    if (cond->class != astEmpty) {
        operand Condition = emitterValue(ctx, cond, operandCreateFlags(conditionUndefined));
        asmBranch(ctx->Asm, Condition, EndLabel);
    }

    /*Do block*/

    emitterCode(ctx, Node->l);
    asmComment(ctx->Asm, "");

    /*Iterate*/

    if (iter->class != astEmpty) {
        operandFree(emitterValue(ctx, iter, operandCreate(operandUndefined)));
        asmComment(ctx->Asm, "");
    }

    /*loopen Sie*/

    asmJump(ctx->Asm, LoopLabel);
    asmLabel(ctx->Asm, EndLabel);

    ctx->labelBreakTo = OldBreakTo;

    debugLeave();
}
