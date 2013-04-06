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
 * Handles any node tag that parserDeclExpr may produce by passing it
 * off to one of the following specialized handlers.
 */
static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, const type* base);

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, const type* returnType);
static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, const type* base);

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
        const type* DT = analyzerDeclNode(ctx, Current, BasicDT);

        if (Current->symbol)
            reportSymbol(Current->symbol);

        else
            reportType(DT);
    }

    debugLeave();
}

const type* analyzerType (struct analyzerCtx* ctx, struct ast* Node) {
    debugEnter("Type");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, BasicDT));

    debugLeave();

    return Node->dt;
}

static void analyzerDeclParam (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclParam");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, BasicDT));

    debugLeave();
}

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node) {
    (void) ctx;

    debugEnter("DeclBasic");

    if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            Node->dt = typeCreateBasic(Node->symbol);

        else {
            debugErrorUnhandled("analyzerDeclBasic", "literal tag", literalTagGetStr(Node->litTag));
            Node->dt = typeCreateInvalid();
        }

    } else {
        debugErrorUnhandled("analyzerDeclBasic", "AST tag", astTagGetStr(Node->tag));
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, const type* base) {
    if (Node->tag == astInvalid) {
        debugMsg("Invalid");
        return Node->dt = typeCreateInvalid();

    } else if (Node->tag == astEmpty)
        return Node->dt = typeDeepDuplicate(base);

    else if (Node->tag == astBOP) {
        if (!strcmp(Node->o, "="))
            return analyzerDeclAssignBOP(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astUOP) {
        if (!strcmp(Node->o, "*"))
            return analyzerDeclPtrUOP(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astCall)
        return analyzerDeclCall(ctx, Node, base);

    else if (Node->tag == astIndex)
        return analyzerDeclIndex(ctx, Node, base);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            return analyzerDeclIdentLiteral(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "literal tag", literalTagGetStr(Node->litTag));
            return Node->dt = typeCreateInvalid();
        }

    } else {
        debugErrorUnhandled("analyzerDeclNode", "AST tag", astTagGetStr(Node->tag));
        return Node->dt = typeCreateInvalid();
    }
}

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, const type* base) {
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

static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclPtrUOP");

    Node->dt = typeCreatePtr(typeDeepDuplicate(base));
    const type* DT = analyzerDeclNode(ctx, Node->r, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, const type* returnType) {
    debugEnter("DeclCall");

    /*Param types*/

    type** paramTypes = malloc(Node->children*sizeof(type*));

    int i = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDeclParam(ctx, Current);
        paramTypes[i++] = typeDeepDuplicate(Current->dt);
    }

    /* */

    Node->dt = typeCreateFunction(typeDeepDuplicate(returnType), paramTypes, Node->children);
    const type* DT = analyzerDeclNode(ctx, Node->l, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclIndex");

    if (Node->r->tag == astEmpty)
        Node->dt = typeCreatePtr(typeDeepDuplicate(base));

    else {
        /*!!!*/
        analyzerValue(ctx, Node->r);
        int size =      Node->r->tag == astLiteral
                     && Node->r->litTag == literalInt
                   ? *(int*) Node->r->literal
                   : 10; //lol

        Node->dt = typeCreateArray(typeDeepDuplicate(base), size);
    }

    const type* DT = analyzerDeclNode(ctx, Node->l, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, const type* base) {
    (void) ctx;

    debugEnter("DeclIdentLiteral");

    Node->dt = typeDeepDuplicate(base);

    if (!Node->symbol->dt)
        Node->symbol->dt = Node->dt;

    /*Not the first declaration of this symbol, check type matches*/
    else if (!typeIsEqual(Node->symbol->dt, base))
        errorConflictingDeclarations(ctx, Node, Node->symbol, Node->dt);

    debugLeave();

    return Node->symbol->dt;
}
