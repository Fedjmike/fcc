#include "../inc/analyzer-decl.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"

#include "../inc/analyzer.h"
#include "../inc/analyzer-value.h"

#include "stdlib.h"
#include "string.h"

static void analyzerDeclParam (analyzerCtx* ctx, ast* Node);

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node);

/**
 * Handles any node class that parserDeclExpr may produce by passing it
 * off to one of the following specialized handlers.
 */
static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, type* base);

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, type* base);
static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, type* base);
static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, type* returnType);
static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, type* base);
static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, type* base);

void analyzerDeclStruct (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclStruct");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current);
    }

    debugLeave();
}

void analyzerDecl (analyzerCtx* ctx, ast* Node) {
    debugEnter("Decl");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDeclNode(ctx, Current, typeDeepDuplicate(BasicDT));
    }

    debugLeave();
}

static void analyzerDeclParam (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclParam");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, typeDeepDuplicate(BasicDT)));

    debugLeave();
}

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node) {
    (void) ctx;

    debugEnter("DeclBasic");

    if (Node->class == astLiteral) {
        if (Node->litClass == literalIdent)
            Node->dt = typeCreateBasic(Node->symbol);

        else {
            debugErrorUnhandled("analyzerDeclBasic", "literal class", literalClassGetStr(Node->litClass));
            Node->dt = typeCreateInvalid();
        }

    } else {
        debugErrorUnhandled("analyzerDeclBasic", "AST class", astClassGetStr(Node->class));
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, type* base) {
    if (Node->class == astBOP) {
        if (!strcmp(Node->o, "="))
            return analyzerDeclAssignBOP(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->class == astUOP) {
        if (!strcmp(Node->o, "*"))
            return analyzerDeclPtrUOP(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->class == astCall)
        return analyzerDeclCall(ctx, Node, base);

    else if (Node->class == astIndex)
        return analyzerDeclIndex(ctx, Node, base);

    else if (Node->class == astLiteral) {
        if (Node->litClass == literalIdent)
            return analyzerDeclIdentLiteral(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "literal class", literalClassGetStr(Node->litClass));
            return Node->dt = typeCreateInvalid();
        }

    } else {
        debugErrorUnhandled("analyzerDeclNode", "AST class", astClassGetStr(Node->class));
        return Node->dt = typeCreateInvalid();
    }
}

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, type* base) {
    debugEnter("DeclAssignBOP");

    const type* L = analyzerDeclNode(ctx, Node->l, base);
    const type* R = analyzerValue(ctx, Node->r);

    if (!typeIsCompatible(R, L))
        analyzerErrorExpectedType(ctx, Node->r, "variable initialization", L, R);

    //const type* DT;

    //if (typeIs)

    debugLeave();

    return L;
}

static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, type* base) {
    debugEnter("DeclPtrUOP");

    Node->dt = typeCreatePtr(base);
    const type* DT = analyzerDeclNode(ctx, Node->r, typeDeepDuplicate(Node->dt));

    debugLeave();

    return DT;
}

static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, type* returnType) {
    debugEnter("DeclCall");

    /*Param types*/

    type** paramTypes = calloc(Node->children, sizeof(type*));

    int i = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDeclParam(ctx, Current);
        paramTypes[i++] = Current->dt;
    }

    /* */

    Node->dt = typeCreateFunction(returnType, paramTypes, Node->children);
    const type* DT = analyzerDeclNode(ctx, Node->l, typeDeepDuplicate(Node->dt));

    debugLeave();

    return DT;
}

static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, type* base) {
    debugEnter("DeclIndex");

    if (Node->r->class == astEmpty)
        Node->dt = typeCreatePtr(base);

    else {
        /*!!!*/
        analyzerValue(ctx, Node->r);
        int size =      Node->r->class == astLiteral
                     && Node->r->litClass == literalInt
                   ? *(int*) Node->r->literal
                   : 10; //lol

        Node->dt = typeCreateArray(base, size);
    }

    const type* DT = analyzerDeclNode(ctx, Node->l, typeDeepDuplicate(Node->dt));

    debugLeave();

    return DT;
}

static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, type* base) {
    (void) ctx;

    debugEnter("DeclIdentLiteral");

    Node->symbol->dt = typeDeepDuplicate(base);

    debugLeave();

    return Node->symbol->dt;
}
