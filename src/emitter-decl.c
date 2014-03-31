#include "../inc/emitter-decl.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/architecture.h"
#include "../inc/reg.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "../inc/emitter.h"
#include "../inc/emitter-value.h"

static void emitterDeclBasic (emitterCtx* ctx, ast* Node);
static void emitterStructOrUnion (emitterCtx* ctx, sym* record, int nextOffset);
static void emitterEnum (emitterCtx* ctx, sym* Symbol);

static void emitterDeclNode (emitterCtx* ctx, ast* Node);
static void emitterDeclAssignBOP (emitterCtx* ctx, const ast* Node);
static void emitterDeclName (emitterCtx* ctx, const ast* Node);

void emitterDecl (emitterCtx* ctx, const ast* Node) {
    debugEnter("Decl");

    emitterDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        emitterDeclNode(ctx, Current);

    debugLeave();
}

static void emitterDeclBasic (emitterCtx* ctx, ast* Node) {
    if (Node->tag == astStruct || Node->tag == astUnion)
        emitterStructOrUnion(ctx, Node->symbol, 0);

    else if (Node->tag == astEnum)
        emitterEnum(ctx, Node->symbol);

    else if (Node->tag == astConst)
        emitterDeclBasic(ctx, Node->r);

    else if (Node->tag == astLiteral)
        ;

    else
        debugErrorUnhandled("emitterDeclBasic", "AST tag", astTagGetStr(Node->tag));
}

static void emitterStructOrUnion (emitterCtx* ctx, sym* record, int nextOffset) {
    debugEnter("StructOrUnion");

    record->size = 0;

    for (sym* Current = record->firstChild;
         Current;
         Current = Current->nextSibling) {
        int fieldSize;

        if (Current->tag == symId) {
            fieldSize = typeGetSize(ctx->arch, Current->dt);

        } else if (Current->tag == symStruct || Current->tag == symUnion) {
            bool anonymous = Current->ident[0] == 0;
            emitterStructOrUnion(ctx, Current, anonymous ? nextOffset : 0);
            fieldSize = Current->size;

        } else if (Current->tag == symEnum) {
            emitterEnum(ctx, Current);
            continue;

        } else {
            debugErrorUnhandled("emitterStructOrUnion", "symbol tag", symTagGetStr(Current->tag));
            continue;
        }

        if (record->tag == symStruct) {
            Current->offset = nextOffset;
            /*Add the size of this field, rounded up to the nearest word boundary*/
            int alignment = ctx->arch->wordsize;
            fieldSize = ((fieldSize-1)/alignment+1)*alignment;
            record->size += fieldSize;
            nextOffset += fieldSize;

        } else /*(record->tag == symUnion)*/ {
            Current->offset = nextOffset;
            record->size = max(record->size, fieldSize);
        }

        reportSymbol(Current);
    }

    reportSymbol(record);

    debugLeave();
}

static void emitterEnum (emitterCtx* ctx, sym* Symbol) {
    debugEnter("Enum");

    Symbol->size = ctx->arch->wordsize;
    reportSymbol(Symbol);

    debugLeave();
}

static void emitterDeclNode (emitterCtx* ctx, ast* Node) {
    if (Node->tag == astInvalid || Node->tag == astEmpty)
        ;

    else if (Node->tag == astBOP) {
        if (Node->o == opAssign)
            emitterDeclAssignBOP(ctx, Node);

        else
            debugErrorUnhandled("emitterDeclNode", "operator", opTagGetStr(Node->o));

    } else  if (Node->tag == astConst)
        emitterDeclNode(ctx, Node->r);

    else if (Node->tag == astUOP) {
        if (Node->o == opDeref)
            emitterDeclNode(ctx, Node->r);

        else
            debugErrorUnhandled("emitterDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astCall)
        /*Nothing to do with the params*/
        emitterDeclNode(ctx, Node->l);

    else if (Node->tag == astIndex)
        /*The emitter does nothing the size of the array, so only go
          down the left branch*/
        emitterDeclNode(ctx, Node->l);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            emitterDeclName(ctx, Node);

        else
            debugErrorUnhandled("emitterDeclNode", "literal tag", literalTagGetStr(Node->litTag));

    } else
        debugErrorUnhandled("emitterDeclNode", "AST tag", astTagGetStr(Node->tag));
}

static void emitterDeclAssignBOP (emitterCtx* ctx, const ast* Node) {
    debugEnter("DeclAssignBOP");

    /*The emitter doesn't need to trace the RHS*/
    emitterDeclNode(ctx, Node->l);

    asmEnter(ctx->Asm);
    operand L = operandCreateMem(&regs[regRBP],
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch,
                                             Node->symbol->dt));
    asmLeave(ctx->Asm);

    if (Node->r->tag == astLiteral && Node->r->litTag == literalInit)
        emitterInitOrCompoundLiteral(ctx, Node->r, L);

    else {
        if (Node->symbol->storage == storageAuto) {
            asmEnter(ctx->Asm);
            operand R = emitterValue(ctx, Node->r, requestOperable);
            asmLeave(ctx->Asm);
            asmMove(ctx->Asm, L, R);
            operandFree(R);

        } else
            debugErrorUnhandled("emitterDeclAssignBOP", "storage tag", storageTagGetStr(Node->symbol->storage));
    }

    debugLeave();
}

static void emitterDeclName (emitterCtx* ctx, const ast* Node) {
    debugEnter("DeclName");

    if (Node->symbol->label == 0)
        ctx->arch->symbolMangler(Node->symbol);

    debugLeave();
}
