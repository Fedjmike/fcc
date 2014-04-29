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

static void emitterDeclNode (emitterCtx* ctx, irBlock** block, ast* Node);
static void emitterDeclAssignBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static void emitterDeclName (emitterCtx* ctx, const ast* Node);

void emitterDecl (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("Decl");

    emitterDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        emitterDeclNode(ctx, block, Current);

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

    /*For every field*/
    for (int n = 0; n < record->children.length; n++) {
        sym* field = vectorGet(&record->children, n);
        int fieldSize;

        /*Find the size of the field*/

        if (field->tag == symId) {
            fieldSize = typeGetSize(ctx->arch, field->dt);

        } else if (field->tag == symStruct || field->tag == symUnion) {
            bool anonymous = !field->ident[0];
            emitterStructOrUnion(ctx, field, anonymous ? nextOffset : 0);
            fieldSize = field->size;

        } else if (field->tag == symEnum) {
            emitterEnum(ctx, field);
            continue;

        } else {
            debugErrorUnhandled("emitterStructOrUnion", "symbol tag", symTagGetStr(field->tag));
            continue;
        }

        /*Set the field offset and update the record size*/

        field->offset = nextOffset;
        reportSymbol(field);

        if (record->tag == symStruct) {
            /*Add the size of this field, rounded up to the nearest word boundary*/
            int alignment = ctx->arch->wordsize;
            fieldSize = ((fieldSize-1)/alignment+1)*alignment;
            record->size += fieldSize;
            nextOffset += fieldSize;

        } else /*if (record->tag == symUnion)*/
            record->size = max(record->size, fieldSize);
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

static void emitterDeclNode (emitterCtx* ctx, irBlock** block, ast* Node) {
    if (Node->tag == astInvalid || Node->tag == astEmpty)
        ;

    else if (Node->tag == astBOP) {
        if (Node->o == opAssign)
            emitterDeclAssignBOP(ctx, block, Node);

        else
            debugErrorUnhandled("emitterDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astConst) {
        emitterDeclNode(ctx, block, Node->r);

    } else if (Node->tag == astUOP) {
        if (Node->o == opDeref)
            emitterDeclNode(ctx, block, Node->r);

        else
            debugErrorUnhandled("emitterDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astCall) {
        /*Nothing to do with the params*/
        emitterDeclNode(ctx, block, Node->l);

    } else if (Node->tag == astIndex) {
        /*The emitter does nothing the to size of the array, so only go
          down the left branch*/
        emitterDeclNode(ctx, block, Node->l);

    } else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            emitterDeclName(ctx, Node);

        else
            debugErrorUnhandled("emitterDeclNode", "literal tag", literalTagGetStr(Node->litTag));

    } else
        debugErrorUnhandled("emitterDeclNode", "AST tag", astTagGetStr(Node->tag));
}

static void emitterDeclAssignBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    debugEnter("DeclAssignBOP");

    emitterDeclNode(ctx, block, Node->l);

    operand L = operandCreateMem(&regs[regRBP],
                                 Node->symbol->offset,
                                 typeGetSize(ctx->arch,
                                             Node->symbol->dt));

    if (Node->r->tag == astLiteral && Node->r->litTag == literalInit)
        emitterCompoundInit(ctx, block, Node->r, L);

    else {
        if (Node->symbol->storage == storageAuto)
            emitterValueSuggest(ctx, block, Node->r, &L);

        else
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
