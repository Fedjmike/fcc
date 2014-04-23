#include "../inc/analyzer-decl.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"

#include "../inc/eval.h"

#include "../inc/analyzer.h"
#include "../inc/analyzer-value.h"

#include "stdlib.h"

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node);
static void analyzerStruct (analyzerCtx* ctx, ast* Node);
static void analyzerUnion (analyzerCtx* ctx, ast* Node);
static void analyzerEnum (analyzerCtx* ctx, ast* Node);

/**
 * Handles any node tag that parserDeclExpr may produce by passing it
 * off to one of the following specialized handlers.
 */
static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, const type* base);

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerConst (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, const type* returnType);
static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, const type* base);

void analyzerDecl (analyzerCtx* ctx, ast* Node) {
    debugEnter("Decl");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        const type* R = analyzerDeclNode(ctx, Current, BasicDT);

        /*Complete? Avoid complaining about typedefs and the like
          (they don't need to be complete)*/
        if (   Current->symbol && Current->symbol->tag == symId
            && !typeIsComplete(R))
            errorIncompleteDecl(ctx, Current);
    }

    debugLeave();
}

const type* analyzerType (analyzerCtx* ctx, struct ast* Node) {
    debugEnter("Type");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, BasicDT));

    debugLeave();

    return Node->dt;
}

void analyzerParam (analyzerCtx* ctx, ast* Node) {
    debugEnter("Param");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, BasicDT));

    debugLeave();
}

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclBasic");

    if (Node->tag == astStruct)
        analyzerStruct(ctx, Node);

    else if (Node->tag == astUnion)
        analyzerUnion(ctx, Node);

    else if (Node->tag == astEnum)
        analyzerEnum(ctx, Node);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            Node->dt = typeCreateBasic(Node->symbol);

        else {
            debugErrorUnhandled("analyzerDeclBasic", "literal tag", literalTagGetStr(Node->litTag));
            Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astConst) {
        Node->dt = typeDeepDuplicate(analyzerDeclBasic(ctx, Node->r));
        Node->dt->qual.isConst = true;

    } else {
        if (Node->tag != astInvalid)
            debugErrorUnhandled("analyzerDeclBasic", "AST tag", astTagGetStr(Node->tag));

        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static void analyzerStruct (analyzerCtx* ctx, ast* Node) {
    debugEnter("Struct");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current);
    }

    Node->dt = typeCreateBasic(Node->symbol);

    /*TODO: check compatiblity
            of? redecls or something?*/

    debugLeave();
}

static void analyzerUnion (analyzerCtx* ctx, ast* Node) {
    debugEnter("Union");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current);
    }

    Node->dt = typeCreateBasic(Node->symbol);

    debugLeave();
}

static void analyzerEnum (analyzerCtx* ctx, ast* Node) {
    debugEnter("Enum");

    Node->dt = typeCreateBasic(Node->symbol);

    int nextConst = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        if (Current->tag == astBOP && Current->o == opAssign) {
            analyzerValue(ctx, Current->r);
            evalResult constant = eval(ctx->arch, Current->r);

            if (constant.known)
                nextConst = constant.value;

            else
                errorCompileTimeKnown(ctx, Current->r, Current->l->symbol, "enum constant");

        } else if (   (Current->tag != astLiteral || Current->litTag != literalIdent)
                   && Current->tag != astInvalid)
            debugErrorUnhandled("analyzerEnum", "AST tag", astTagGetStr(Current->tag));

        if (Current->symbol) {
            analyzerDeclIdentLiteral(ctx, Current, Node->dt);
            Current->symbol->constValue = nextConst++;
        }
    }

    debugLeave();
}

static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, const type* base) {
    if (Node->tag == astInvalid)
        debugMsg("Invalid");

    else if (Node->tag == astBOP) {
        if (Node->o == opAssign)
            return analyzerDeclAssignBOP(ctx, Node, base);

        else
            debugErrorUnhandled("analyzerDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astUOP) {
        if (Node->o == opDeref)
            return analyzerDeclPtrUOP(ctx, Node, base);

        else
            debugErrorUnhandled("analyzerDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            return analyzerDeclIdentLiteral(ctx, Node, base);

        else
            debugErrorUnhandled("analyzerDeclNode", "literal tag", literalTagGetStr(Node->litTag));

    } else if (Node->tag == astEmpty)
        return Node->dt = typeDeepDuplicate(base);

    else if (Node->tag == astConst)
        return analyzerConst(ctx, Node, base);

    else if (Node->tag == astCall)
        return analyzerDeclCall(ctx, Node, base);

    else if (Node->tag == astIndex)
        return analyzerDeclIndex(ctx, Node, base);

    else
        debugErrorUnhandled("analyzerDeclNode", "AST tag", astTagGetStr(Node->tag));

    /*Fall through for error states*/

    if (Node->symbol)
        Node->symbol->dt = typeCreateInvalid();

    return Node->dt = typeCreateInvalid();
}

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclAssignBOP");

    const type* L = analyzerDeclNode(ctx, Node->l, base);

    /*Struct/array initializer?*/
    if (Node->r->tag == astLiteral && Node->r->litTag == literalInit)
        analyzerCompoundInit(ctx, Node->r, L, true);

    else {
        const type* R = analyzerValue(ctx, Node->r);

        if (!typeIsAssignment(L))
            errorTypeExpected(ctx, Node->l, opTagGetStr(Node->o), "assignable type");

        else if (!typeIsCompatible(R, L))
            errorInitMismatch(ctx, Node->l, Node->r);
    }

    Node->dt = typeDeepDuplicate(L);

    debugLeave();

    return Node->dt;
}

static const type* analyzerConst (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("Const");

    Node->dt = typeDeepDuplicate(base);

    if (Node->dt->qual.isConst)
        errorAlreadyConst(ctx, Node);

    else if (typeIsArray(Node->dt) || typeIsFunction(Node->dt))
        errorIllegalConst(ctx, Node);

    else
        Node->dt->qual.isConst = true;

    const type* DT = analyzerDeclNode(ctx, Node->r, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclPtrUOP");

    Node->dt = typeCreatePtr(typeDeepDuplicate(base));
    const type* DT = analyzerDeclNode(ctx, Node->r, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, const type* returnType) {
    debugEnter("DeclCall");

    if (!typeIsComplete(returnType))
        errorIncompleteReturnDecl(ctx, Node, returnType);

    /*Param types*/

    bool variadic = false;
    type** paramTypes = calloc(Node->children, sizeof(type*));
    ast* Current;
    int i;

    for (Current = Node->firstChild, i = 0;
         Current;
         Current = Current->nextSibling) {
        if (Current->tag == astEllipsis) {
            variadic = true;
            debugMsg("Ellipsis");

        } else if (Current->tag == astParam) {
            analyzerParam(ctx, Current);
            paramTypes[i++] = typeDeepDuplicate(Current->dt);

            if (!typeIsComplete(Current->dt))
                errorIncompleteParamDecl(ctx, Current, Node, i);

        } else
            debugErrorUnhandled("analyzerDeclCall", "AST tag", astTagGetStr(Current->tag));
    }

    /* */

    Node->dt = typeCreateFunction(typeDeepDuplicate(returnType), paramTypes, i, variadic);
    const type* DT = analyzerDeclNode(ctx, Node->l, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclIndex");

    if (Node->r->tag == astEmpty)
        Node->dt = typeCreatePtr(typeDeepDuplicate(base));

    else {
        analyzerValue(ctx, Node->r);
        evalResult size = eval(ctx->arch, Node->r);

        if (!size.known) {
            errorCompileTimeKnown(ctx, Node->r, Node->l->symbol, "array size");
            size.value = -2;

        } else if (size.value <= 0)
            errorIllegalArraySize(ctx, Node->r, Node->l->symbol, size.value);

        Node->dt = typeCreateArray(typeDeepDuplicate(base), size.value);
    }

    const type* DT = analyzerDeclNode(ctx, Node->l, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclIdentLiteral");

    Node->dt = typeDeepDuplicate(base);

    if (Node->symbol) {
        /*Assign the type*/
        if (!Node->symbol->dt)
            Node->symbol->dt = typeDeepDuplicate(base);

        /*Don't bother checking types if param
          Any conflicts will be reported by the function itself*/
        else if (Node->symbol->tag == symParam)
            ;

        /*Not the first declaration of this symbol, check type matches*/
        else if (!typeIsEqual(Node->symbol->dt, base))
            errorConflictingDeclarations(ctx, Node, Node->symbol, base);

        /*Even if types match, not allowed to be redeclared if a variable*/
        else if (   Node->symbol->tag == symId &&
                 !(   typeIsFunction(base)
                   || Node->symbol->parent->tag == symStruct
                   || Node->symbol->parent->tag == symUnion
                   || Node->symbol->storage == storageExtern))
            errorRedeclared(ctx, Node, Node->symbol);

        reportSymbol(Node->symbol);

    } else
        reportType(base);

    debugLeave();

    return Node->dt;
}
