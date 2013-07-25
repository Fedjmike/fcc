#include "../inc/emitter-decl.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/reg.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "../inc/emitter.h"
#include "../inc/emitter-value.h"

#include "string.h"

static void emitterDeclBasic (emitterCtx* ctx, ast* Node);
static void emitterDeclStruct (emitterCtx* ctx, ast* Node);
static void emitterDeclUnion (emitterCtx* ctx, ast* Node);

static void emitterDeclNode (emitterCtx* ctx, ast* Node);
static void emitterDeclAssignBOP (emitterCtx* ctx, const ast* Node);
static void emitterDeclCall (emitterCtx* ctx, const ast* Node);

static void emitterArrayLiteral (emitterCtx* ctx, const ast* Node, const sym* Symbol);

void emitterDecl (emitterCtx* ctx, const ast* Node) {
    (void) ctx;

    debugEnter("Decl");

    emitterDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        emitterDeclNode(ctx, Current);

    debugLeave();
}

static void emitterDeclBasic (emitterCtx* ctx, ast* Node) {
    if (Node->tag == astDeclStruct)
        emitterDeclStruct(ctx, Node);

    else if (Node->tag == astDeclUnion)
        emitterDeclUnion(ctx, Node);

    else if (Node->tag == astLiteral)
        ;

    else
        debugErrorUnhandled("emitterDeclBasic", "AST tag", astTagGetStr(Node->tag));
}

static void emitterDeclStruct (emitterCtx* ctx, ast* Node) {
    (void) ctx;

    debugEnter("DeclStruct");

    for (sym* Current = Node->symbol->firstChild;
         Current;
         Current = Current->nextSibling) {
        Current->offset = Node->symbol->size;
        /*Add the size of this field, rounded up to the nearest word boundary*/
        int alignment = ctx->arch->wordsize;
        Node->symbol->size += ((typeGetSize(ctx->arch, Current->dt) - 1)/alignment)*alignment + alignment;
        reportSymbol(Current);
    }

    reportSymbol(Node->symbol);

    debugLeave();
}

static void emitterDeclUnion (emitterCtx* ctx, ast* Node) {
    (void) ctx;

    debugEnter("DeclUnion");

    for (sym* Current = Node->symbol->firstChild;
         Current;
         Current = Current->nextSibling) {
        Current->offset = 0;
        Node->symbol->size = max(Node->symbol->size, typeGetSize(ctx->arch, Current->dt));
        reportSymbol(Current);
    }

    reportSymbol(Node->symbol);

    debugLeave();
}

static void emitterDeclNode (emitterCtx* ctx, ast* Node) {
    debugMsg("DeclNode");

    if (Node->tag == astInvalid || Node->tag == astEmpty)
        ;

    else if (Node->tag == astBOP) {
        if (!strcmp(Node->o, "="))
            emitterDeclAssignBOP(ctx, Node);

        else
            debugErrorUnhandled("emitterDeclNode", "operator", Node->o);

    } else if (Node->tag == astUOP) {
        if (!strcmp(Node->o, "*"))
            emitterDeclNode(ctx, Node->r);

        else
            debugErrorUnhandled("emitterDeclNode", "operator", Node->o);

    } else if (Node->tag == astCall)
        emitterDeclCall(ctx, Node);

    else if (Node->tag == astIndex)
        /*The emitter does nothing the size of the array, so only go
          down the left branch*/
        emitterDeclNode(ctx, Node->l);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            ;

        else
            debugErrorUnhandled("emitterDeclNode", "literal tag", literalTagGetStr(Node->litTag));

    } else
        debugErrorUnhandled("emitterDeclNode", "AST tag", astTagGetStr(Node->tag));
}

static void emitterDeclAssignBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("DeclAssignBOP");

    /*The emitter doesn't need to trace the RHS*/
    emitterDeclNode(ctx, Node->l);

    /*Array*/
    if (Node->r->tag == astLiteral && Node->r->litTag == literalArray)
        emitterArrayLiteral(ctx, Node->r, Node->symbol);

    else {
        if (Node->symbol->storage == storageAuto) {
            asmEnter(ctx->Asm);
            operand L = operandCreateMem(&regs[regRBP],
                                         Node->symbol->offset,
                                         typeGetSize(ctx->arch,
                                                     Node->symbol->dt));
            operand R = emitterValue(ctx, Node->r, operandCreate(operandUndefined));
            asmLeave(ctx->Asm);
            asmMove(ctx->Asm, L, R);
            operandFree(R);

        } else
            debugErrorUnhandled("emitterDeclAssignBOP", "storage tag", storageTagGetStr(Node->symbol->storage));
    }

    debugLeave();
}

static void emitterDeclCall (emitterCtx* ctx, const ast* Node) {
    debugEnter("DeclCall");

    /*Nothing to do with the params*/
    emitterDeclNode(ctx, Node->l);

    if (Node->symbol->label.label == 0)
        Node->symbol->label = labelNamed(Node->symbol->ident);

    debugLeave();
}

static void emitterArrayLiteral (emitterCtx* ctx, const ast* Node, const sym* Symbol) {
    debugEnter("ArrayLiteral");

    int n = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling, n++) {
        asmEnter(ctx->Asm);
        operand L = operandCreateMem(&regs[regRBP],
                                     Symbol->offset + typeGetSize(ctx->arch, Symbol->dt->base)*n,
                                     typeGetSize(ctx->arch, Symbol->dt->base));
        operand R = emitterValue(ctx, Current, operandCreate(operandUndefined));
        asmLeave(ctx->Asm);
        asmMove(ctx->Asm, L, R);
        operandFree(R);
    }

    debugLeave();
}
