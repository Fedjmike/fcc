#include "../inc/emitter-decl.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/architecture.h"
#include "../inc/ir.h"

#include "../inc/eval.h"
#include "../inc/emitter.h"
#include "../inc/emitter-value.h"

#include "assert.h"

static void emitterDeclBasic (emitterCtx* ctx, ast* Node);
static void emitterStructOrUnion (emitterCtx* ctx, sym* record, int nextOffset);
static void emitterEnum (emitterCtx* ctx, sym* Symbol);

static void emitterDeclNode (emitterCtx* ctx, irBlock** block, ast* Node);
static void emitterDeclAssignBOP (emitterCtx* ctx, irBlock** block, const ast* Node);
static void emitterDeclCall (emitterCtx* ctx, irBlock** block, const ast* Node);
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
    debugEnter(astTagGetStr(Node->tag));

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

    debugLeave();
}

static void emitterStructOrUnion (emitterCtx* ctx, sym* record, int nextOffset) {
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

        } else {
            assert(record->tag == symUnion);
            record->size = max(record->size, fieldSize);
        }
    }

    reportSymbol(record);
}

static void emitterEnum (emitterCtx* ctx, sym* Symbol) {
    Symbol->size = ctx->arch->wordsize;
    reportSymbol(Symbol);
}

static void emitterDeclNode (emitterCtx* ctx, irBlock** block, ast* Node) {
    debugEnter(astTagGetStr(Node->tag));

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
        emitterDeclCall(ctx, block, Node);

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

    debugLeave();
}

static void emitterDeclAssignBOP (emitterCtx* ctx, irBlock** block, const ast* Node) {
    emitterDeclNode(ctx, block, Node->l);

    if (   Node->symbol->storage == storageStatic
        || Node->symbol->storage == storageExtern) {
        if (Node->r->tag == astLiteral && Node->r->litTag == literalInit)
            debugError("emitterDeclAssignBOP", "compound initializers for static objects unsupported");

        else
            irStaticValue(ctx->ir, Node->symbol->label, Node->symbol->storage == storageExtern,
                          typeGetSize(ctx->arch, Node->symbol->dt), eval(ctx->arch, Node->r).value);

    } else if (Node->symbol->storage == storageAuto) {
        operand L = emitterSymbol(ctx, Node->symbol);

        if (Node->r->tag == astLiteral && Node->r->litTag == literalInit)
            emitterCompoundInit(ctx, block, Node->r, L);

        else {
            emitterValueSuggest(ctx, block, Node->r, &L);
        }

    } else if (Node->symbol->storage != storageExtern)
        debugErrorUnhandled("emitterDeclAssignBOP", "storage tag", storageTagGetStr(Node->symbol->storage));
}

static void emitterDeclCall (emitterCtx* ctx, irBlock** block, const ast* Node) {
    for (ast* param = Node->firstChild;
         param;
         param = param->nextSibling) {
        if (param->tag == astParam) {
            emitterDeclBasic(ctx, param->l);
            emitterDeclNode(ctx, block, param->r);

        } else if (param->tag == astEllipsis)
            ;

        else
            debugErrorUnhandled("emitterDeclCall", "AST tag", astTagGetStr(param->tag));
    }

    emitterDeclNode(ctx, block, Node->l);
}

static void emitterDeclName (emitterCtx* ctx, const ast* Node) {
    /*Function or statically stored variable? Provide a label if none*/
    if (   Node->symbol->tag == symId
        && Node->symbol->label == 0
        && (   Node->symbol->storage == storageStatic
            || Node->symbol->storage == storageExtern))
        ctx->arch->symbolMangler(Node->symbol);

    /*Static declaration without an explicit initializer?*/
    if (   Node->symbol->tag == symId
        && Node->storage == storageStatic
        && !Node->symbol->impl)
        /*Emit, and initialize to zero*/
        irStaticValue(ctx->ir, Node->symbol->label, Node->symbol->storage == storageExtern,
                      typeGetSize(ctx->arch, Node->symbol->dt), 0);
}
