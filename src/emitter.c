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

FILE* Asm;

/*Emitter context*/
operand labelReturnTo;
operand labelBreakTo;

static void emitterModule (ast* Tree);
static void emitterStruct (sym* Symbol);
static void emitterEnum (sym* Symbol);
static void emitterFunction (ast* Node);
static void emitterCode (ast* Node);
static void emitterLine (ast* Node);

static void emitterBranch (ast* Node);
static void emitterLoop (ast* Node);
static void emitterIter (ast* Node);
static void emitterVar (ast* Node);
static void emitterArrayLit (ast* Node, sym* Symbol);

void emitter (ast* Tree, FILE* File) {
    asmInit(File);

    Asm = File;
    fprintf(Asm, ".intel_syntax noprefix\n");

    labelFreeAll();

    emitterModule(Tree);
}

void emitterModule (ast* Tree) {
    puts("Module+");

    /*Resolve struct info*/
    for (sym* Current = Tree->symbol->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->class == symStruct)
            emitterStruct(Current);

        else if (Current->class == symEnum)
            emitterEnum(Current);

        else if (Current->class == symType ||
                 Current->class == symFunction)
            ; //Nothing to do

        else
            printf("emitterModule: unhandled sym class, %d.\n", Current->class);
    }

    /*Functions*/
    for (ast* Current = Tree->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->class == astFunction)
            emitterFunction(Current);

        else
            printf("emitterModule(): unhandled AST class, %d.\n", Current->class);
    }

    puts("-");
}

void emitterStruct (sym* Symbol) {
    puts("Struct+");

    for (sym* Current = Symbol->firstChild;
         Current;
         Current = Current->nextSibling) {
        Current->offset = Symbol->size;
        /*Add the size of this field, rounded up to the nearest word boundary*/
        Symbol->size += ((typeGetSize(Current->dt)-1) / 8) + 8;
        reportSymbol(Current);
    }

    reportSymbol(Symbol);

    puts("-");
}

void emitterEnum (sym* Symbol) {
    puts("Enum+");

    for (sym* Current = Symbol->firstChild;
         Current;
         Current = Current->nextSibling) {
        Current->class++;
    }

    puts("-");
}

void emitterFunction (ast* Node) {
    puts("Function+");

    Node->symbol->label = labelNamed(Node->symbol->ident);

    int lastOffset = 16;
    sym* Symbol;

    /*Asign offsets to all the parameters*/
    for (Symbol = Node->symbol->firstChild;
         Symbol && Symbol->class == symPara;
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
    operand EndLabel = labelReturnTo = labelCreate(labelReturn);

    asmComment("");
    asmFnPrologue(labelGet(Node->symbol->label),
                  Node->symbol->lastChild && Node->symbol->lastChild->class != symPara
                        ? -Node->symbol->lastChild->offset : 0);
    emitterCode(Node->r);
    asmFnEpilogue(labelGet(EndLabel));

    puts("-");
}

void emitterCode (ast* Node) {
    asmEnter();

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        emitterLine(Current);
    }

    asmLeave();
}

void emitterLine (ast* Node) {
    puts("Line+");

    asmComment("");

    if (Node->class == astBranch)
        emitterBranch(Node);

    else if (Node->class == astLoop)
        emitterLoop(Node);

    else if (Node->class == astIter)
        emitterIter(Node);

    else if (Node->class == astReturn) {
        emitterValue(Node->r, operandCreateReg(regRAX));
        asmJump(labelReturnTo);

    } else if (Node->class == astVar)
        emitterVar(Node);

    else if (astIsValueClass(Node->class))
        operandFree(emitterValue(Node, operandCreate(operandUndefined)));

    else
        printf("emitterLine(): unhandled AST class, %d.\n", Node->class);

    puts("-");
}

void emitterBranch (ast* Node) {
    puts("Branch+");

    operand ElseLabel = labelCreate(labelUndefined);
    operand EndLabel = labelCreate(labelUndefined);

    /*Compute the condition, requesting it be placed in the flags*/
    asmBranch(emitterValue(Node->firstChild,
                             operandCreateFlags(conditionUndefined)),
              ElseLabel);

    emitterCode(Node->l);

    asmComment("");
    asmJump(EndLabel);
    asmLabel(ElseLabel);

    emitterCode(Node->r);

    asmLabel(EndLabel);

    puts("-");
}

void emitterLoop (ast* Node) {
    puts("Loop+");

    operand LoopLabel = labelCreate(labelUndefined); /*The place to return to loop again (after confirming condition)*/
    operand OldBreakTo = labelBreakTo; /*Push old break label before erasing*/
    operand EndLabel = labelBreakTo = labelCreate(labelUndefined); /*To exit the loop (or break, anywhere)*/

    /*Work out which order the condition and code came in
      => whether this is a while or a do while*/
    bool isDo = Node->l->class == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    if (!isDo)
        asmBranch(emitterValue(cond, operandCreateFlags(conditionUndefined)),
                  EndLabel);

    asmLabel(LoopLabel);
    emitterCode(code);

    asmComment("");

    asmBranch(emitterValue(cond, operandCreateFlags(conditionUndefined)),
              EndLabel);

    asmJump(LoopLabel);
    asmLabel(EndLabel);

    labelBreakTo = OldBreakTo;

    puts("-");
}

void emitterIter (ast* Node) {
    puts("Iter+");

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    operand LoopLabel = labelCreate(labelUndefined);
    operand OldBreakTo = labelBreakTo;
    operand EndLabel = labelBreakTo = labelCreate(labelUndefined);

    /*Initialize stuff*/

    if (init->class == astVar)
        emitterVar(init);

    else if (astIsValueClass(init->class))
        operandFree(emitterValue(init, operandCreate(operandUndefined)));

    else if (init->class != astEmpty)
        printf("emitterIter(): unhandled AST class, %d.\n", init->class);

    asmComment("");

    /*Check condition*/

    asmLabel(LoopLabel);

    if (cond->class != astEmpty) {
        operand Condition = emitterValue(cond, operandCreateFlags(conditionUndefined));
        asmBranch(Condition, EndLabel);
    }

    /*Do block*/

    emitterCode(Node->l);

    /*Iterate*/

    asmComment("");

    if (iter->class != astEmpty)
        operandFree(emitterValue(iter, operandCreate(operandUndefined)));

    /*loopen Sie*/

    asmComment("");
    asmJump(LoopLabel);
    asmLabel(EndLabel);

    labelBreakTo = OldBreakTo;

    puts("-");
}

void emitterVar (ast* Node) {
    puts("Var+");

    /*Is there an initial value assigned?*/
    if (Node->r) {
        if (Node->r->class == astArrayLit)
            emitterArrayLit(Node->r, Node->symbol);

        else if (Node->symbol->storage == storageAuto) {
            asmEnter();
            operand L = operandCreateMem(regRBP, Node->symbol->offset, Node->symbol->dt.basic->size);
            operand R = emitterValue(Node->r, operandCreate(operandUndefined));
            asmLeave();
            asmMove(L, R);
            operandFree(R);

        } else
            puts("finish me");
    }

    if (Node->l)
        emitterVar(Node->l);

    puts("-");
}

void emitterArrayLit (ast* Node, sym* Symbol) {
    puts("ArrayLit+");

    int n = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling, n++) {
        asmEnter();
        operand L = operandCreateMem(regRBP,
                                     Symbol->offset + Symbol->dt.basic->size*n,
                                     Symbol->dt.basic->size);
        operand R = emitterValue(Current, operandCreate(operandUndefined));
        asmLeave();
        asmMove(L, R);
        operandFree(R);
    }

    puts("-");
}
