#include "string.h"

#include "../inc/debug.h"
#include "../inc/sym.h"
#include "../inc/analyzer.h"
#include "../inc/analyzer-value.h"

static type analyzerNumericBOP (analyzerCtx* ctx, ast* Node);
static type analyzerComparisonBOP (analyzerCtx* ctx, ast* Node);
static type analyzerMemberBOP (analyzerCtx* ctx, ast* Node);
static type analyzerCommaBOP (analyzerCtx* ctx, ast* Node);
static type analyzerUOP (analyzerCtx* ctx, ast* Node);
static type analyzerTernary (analyzerCtx* ctx, ast* Node);
static type analyzerIndex (analyzerCtx* ctx, ast* Node);
static type analyzerCall (analyzerCtx* ctx, ast* Node);
static type analyzerLiteral (analyzerCtx* ctx, ast* Node);
static type analyzerArrayLiteral (analyzerCtx* ctx, ast* Node);

/**
 * Returns whether the (binary) operator is one that can only act on
 * numeric types (e.g. int, char, not bool, not x*)
 */
static bool isNumericBOP (char* o) {
    return !strcmp(o, "+") || !strcmp(o, "-") ||
           !strcmp(o, "*") || !strcmp(o, "/") ||
           !strcmp(o, "%") ||
           !strcmp(o, "&") || !strcmp(o, "|") ||
           !strcmp(o, "^") ||
           !strcmp(o, "<<") || !strcmp(o, ">>") ||
           !strcmp(o, "+=") || !strcmp(o, "-=") ||
           !strcmp(o, "*=") || !strcmp(o, "/=") ||
           !strcmp(o, "%=") ||
           !strcmp(o, "&=") || !strcmp(o, "|=") ||
           !strcmp(o, "^=") ||
           !strcmp(o, "<<=") || !strcmp(o, ">>=");
}

/**
 * Is it an ordinal operator (defines an ordering...)?
 */
static bool isOrdinalBOP (char* o) {
    return !strcmp(o, ">") || !strcmp(o, "<") ||
           !strcmp(o, ">=") || !strcmp(o, "<=");
}

static bool isEqualityBOP (char* o) {
    return !strcmp(o, "==") || !strcmp(o, "!=");
}

static bool isAssignmentBOP (char* o) {
    return !strcmp(o, "=") ||
           !strcmp(o, "+=") || !strcmp(o, "-=") ||
           !strcmp(o, "*=") || !strcmp(o, "/=") ||
           !strcmp(o, "%=") ||
           !strcmp(o, "&=") || !strcmp(o, "|=") ||
           !strcmp(o, "^=") ||
           !strcmp(o, "<<=") || !strcmp(o, ">>=");
}

/**
 * Does this operator access struct/class members of its LHS?
 */
static bool isMemberBOP (char* o) {
    return !strcmp(o, ".") || !strcmp(o, "->");
}

/**
 * Does this member dereference its LHS?
 */
static bool isDerefBOP (char* o) {
    return !strcmp(o, "->");
}

/**
 * Is this the ',' operator? Yes, a bit trivial, this class
 */
static bool isCommaBOP (char* o) {
    return !strcmp(o, ",");
}

type analyzerValue (analyzerCtx* ctx, ast* Node) {
    if (Node->class == astBOP) {
        if (isNumericBOP(Node->o))
            return analyzerNumericBOP(ctx, Node);

        else if (isOrdinalBOP(Node->o) || isEqualityBOP(Node->o))
            return analyzerComparisonBOP(ctx, Node);

        else if (isMemberBOP(Node->o))
            return analyzerMemberBOP(ctx, Node);

        else if (isCommaBOP(Node->o))
            return analyzerCommaBOP(ctx, Node);

        else {
            debugErrorUnhandled("analyzerValue", "operator", Node->o);
            return Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
        }

    } else if (Node->class == astUOP)
        return analyzerUOP(ctx, Node);

    else if (Node->class == astTOP)
        return analyzerTernary(ctx, Node);

    else if (Node->class == astIndex)
        return analyzerIndex(ctx, Node);

    else if (Node->class == astCall)
        return analyzerCall(ctx, Node);

    else if (Node->class == astLiteral) {
        if (Node->litClass == literalArray)
            return analyzerArrayLiteral(ctx, Node);

        else
            return analyzerLiteral(ctx, Node);

    } else {
        debugErrorUnhandled("analyzerValue", "AST class", astClassGetStr(Node->class));
        return Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }
}

static type analyzerNumericBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("BOP");

    type L = analyzerValue(ctx, Node->l);
    type R = analyzerValue(ctx, Node->r);

    /*Check that the operation are allowed on the operands given*/

    if (!typeIsNumeric(L) || !typeIsNumeric(R))
        analyzerErrorOp(ctx, Node, Node->o, "numeric type",
                        !typeIsNumeric(L) ? Node->l : Node->r,
                        !typeIsNumeric(L) ? L : R);

    if (isAssignmentBOP(Node->o)) {
        if (!typeIsAssignment(L) || !typeIsAssignment(R))
            analyzerErrorOp(ctx, Node, Node->o, "assignable type",
                            !typeIsAssignment(L) ? Node->l : Node->r,
                            !typeIsAssignment(L) ? L : R);

        if (!typeIsLValue(L))
            analyzerErrorOp(ctx, Node, Node->o, "lvalue", Node->l, L);
    }

    /*Work out the type of the result*/

    if (typeIsCompatible(L, R)) {
        /*The type of the right hand side
          (assignment does not return an lvalue)*/
        if (isAssignmentBOP(Node->o))
            Node->dt = typeCreateFrom(R);

        else
            Node->dt = typeCreateFromTwo(L, R);

    } else {
        analyzerErrorMismatch(ctx, Node, Node->o, L, R);
        Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }

    debugLeave();

    return Node->dt;
}

static type analyzerComparisonBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("ComparisonBOP");

    type L = analyzerValue(ctx, Node->l);
    type R = analyzerValue(ctx, Node->r);

    /*Allowed?*/

    if (isOrdinalBOP(Node->o)) {
        if (!typeIsOrdinal(L) || !typeIsOrdinal(R))
            analyzerErrorOp(ctx, Node, Node->o, "comparable type",
                            !typeIsOrdinal(L) ? Node->l : Node->r,
                            !typeIsOrdinal(L) ? L : R);

    } else /*if (isEqualityBOP(Node->o))*/
        if (!typeIsEquality(L) || !typeIsEquality(R))
            analyzerErrorOp(ctx, Node, Node->o, "comparable type",
                            !typeIsEquality(L) ? Node->l : Node->r,
                            !typeIsEquality(L) ? L : R);

    /*Result*/

    if (typeIsCompatible(L, R))
        Node->dt = typeCreateFromTwo(L, R);

    else {
        analyzerErrorMismatch(ctx, Node, Node->o, L, R);
        Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }

    debugLeave();

    return Node->dt;
}

static type analyzerMemberBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("MemberBOP");

    type L = analyzerValue(ctx, Node->l);
    type R = analyzerValue(ctx, Node->r);

    /*Operator allowed?*/

    /* -> */
    if (isDerefBOP(Node->o)) {
        if (!typeIsRecord(typeCreateLValueFromPtr(L)))
            analyzerErrorOp(ctx, Node, Node->o, "structure pointer", Node->l, L);

        else if (!typeIsPtr(L))
            analyzerErrorOp(ctx, Node, Node->o, "pointer", Node->l, L);

    /* . */
    } else
        if (!typeIsRecord(L))
            analyzerErrorOp(ctx, Node, Node->o, "structure type", Node->l, L);

    /*Return type: the field*/

    Node->dt = R;

    debugLeave();

    return Node->dt;
}

static type analyzerCommaBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("CommaBOP");

    type R = analyzerValue(ctx, Node->r);

    if (!typeIsVoid(R))
        Node->dt = typeCreateFrom(R);

    else {
        analyzerErrorOp(ctx, Node, Node->o, "non-void", Node->r, R);
        Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }

    debugLeave();

    return Node->dt;
}

static type analyzerUOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("UOP");

    type R = analyzerValue(ctx, Node->r);
    Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);

    /*Numeric operator*/
    if (!strcmp(Node->o, "+") || !strcmp(Node->o, "-") ||
        !strcmp(Node->o, "++") || !strcmp(Node->o, "--") ||
        !strcmp(Node->o, "!") ||
        !strcmp(Node->o, "~")) {
        if (!typeIsNumeric(R))
            analyzerErrorOp(ctx, Node, Node->o, "numeric type", Node->r, R);

        /*Assignment operator*/
        else if (!strcmp(Node->o, "++") || !strcmp(Node->o, "--")) {
            if (typeIsLValue(R))
                Node->dt = typeCreateFrom(R);

            else
                analyzerErrorOp(ctx, Node, Node->o, "lvalue", Node->r, R);

        } else
            Node->dt = typeCreateFrom(R);

    /*Dereferencing a pointer*/
    } else if (!strcmp(Node->o, "*")) {
        if (typeIsPtr(R))
            Node->dt = typeCreateLValueFromPtr(R);

        else
            analyzerErrorOp(ctx, Node, Node->o, "pointer", Node->r, R);

    /*Referencing an lvalue*/
    } else if (!strcmp(Node->o, "&")) {
        if (typeIsLValue(R))
            Node->dt = typeCreatePtrFromLValue(R);

        else
            analyzerErrorOp(ctx, Node, Node->o, "lvalue", Node->r, R);

    } else
        debugErrorUnhandled("analyzerUOP", "operator", Node->o);

    debugLeave();

    return Node->dt;
}

static type analyzerTernary (analyzerCtx* ctx, ast* Node) {
    debugEnter("Ternary");

    type Cond = analyzerValue(ctx, Node->firstChild);
    type L = analyzerValue(ctx, Node->l);
    type R = analyzerValue(ctx, Node->r);

    /*Operation allowed*/

    if (!typeIsNumeric(Cond))
        analyzerErrorOp(ctx, Node, "?:", "condition value", Node->firstChild, Cond);

    /*Result types match => return type*/

    if (!typeIsCompatible(L, R))
        Node->dt = typeCreateUnified(L, R);

    else {
        analyzerErrorMismatch(ctx, Node, "?:", L, R);
        Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }

    debugLeave();

    return Node->dt;
}

static type analyzerIndex (analyzerCtx* ctx, ast* Node) {
    debugEnter("Index");

    type L = analyzerValue(ctx, Node->l);
    type R = analyzerValue(ctx, Node->r);

    if (!typeIsNumeric(R))
        analyzerErrorOp(ctx, Node, "[]", "numeric index", Node->r, R);

    if (typeIsArray(L))
        Node->dt = typeCreateElementFromArray(L);

    else if (typeIsPtr(L))
        Node->dt = typeCreateLValueFromPtr(L);

    else {
        analyzerErrorOp(ctx, Node, "[]", "array or pointer", Node->l, L);
        Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }

    debugLeave();

    return Node->dt;
}

static type analyzerCall (analyzerCtx* ctx, ast* Node) {
    debugEnter("Call");

    /*Allowed?*/

    /*Is it actually a function?*/
    if (Node->symbol->class != symFunction)
        analyzerErrorOp(ctx, Node, "()", "function", Node->firstChild, Node->symbol->dt);

    /*Right number of params?*/
    else if (Node->symbol->params != Node->children)
        analyzerErrorDegree(ctx, Node, "parameters",
                            Node->symbol->params, Node->children,
                            Node->symbol->ident);

    /*Do the parameter types match?*/
    else {
        ast* cNode;
        sym* cParam;
        int n;

        /*Traverse down both lists at once, checking types, leaving
          once either ends (we already know they have the same length)*/
        for (cNode = Node->firstChild, cParam = Node->symbol->firstChild, n = 0;
             cNode && cParam;
             cNode = cNode->nextSibling, cParam = cParam->nextSibling, n++) {
            type Param = analyzerValue(ctx, cNode);

            if (!typeIsCompatible(Param, cParam->dt))
                analyzerErrorParamMismatch(ctx, Node, n, cParam->dt, Param);
        }
    }

    /*Return type*/

    Node->dt = Node->symbol->dt;

    debugLeave();

    return Node->dt;
}

static type analyzerLiteral (analyzerCtx* ctx, ast* Node) {
    debugEnter("Literal");

    if (Node->litClass == literalInt)
        Node->dt = typeCreateFromBasic(ctx->types[builtinInt]);

    else if (Node->litClass == literalBool)
        Node->dt = typeCreateFromBasic(ctx->types[builtinBool]);

    else if (Node->litClass == literalIdent)
        Node->dt = Node->symbol->dt;

    else {
        debugErrorUnhandled("analyzerLiteral", "AST class", astClassGetStr(Node->class));
        Node->dt = typeCreateFromBasic(ctx->types[builtinVoid]);
    }

    debugLeave();

    return Node->dt;
}

static type analyzerArrayLiteral (analyzerCtx* ctx, ast* Node) {
    debugEnter("ArrayLiteral");

    /*Allowed?*/

    /*TODO: Check element types match*/

    /*Return type*/

    Node->dt = typeCreateArrayFromElement(analyzerValue(ctx, Node->firstChild),
                                          Node->children);

    debugLeave();

    return Node->dt;
}
