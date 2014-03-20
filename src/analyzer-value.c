#include "../inc/analyzer-value.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"

#include "../inc/analyzer.h"
#include "../inc/analyzer-decl.h"

#include "string.h"

static const type* analyzerBOP (analyzerCtx* ctx, ast* Node);
static const type* analyzerComparisonBOP (analyzerCtx* ctx, ast* Node);
static const type* analyzerLogicalBOP (analyzerCtx* ctx, ast* Node);
static const type* analyzerMemberBOP (analyzerCtx* ctx, ast* Node);
static const type* analyzerCommaBOP (analyzerCtx* ctx, ast* Node);

static const type* analyzerUOP (analyzerCtx* ctx, ast* Node);
static const type* analyzerTernary (analyzerCtx* ctx, ast* Node);
static const type* analyzerIndex (analyzerCtx* ctx, ast* Node);
static const type* analyzerCall (analyzerCtx* ctx, ast* Node);
static const type* analyzerCast (analyzerCtx* ctx, ast* Node);
static const type* analyzerSizeof (analyzerCtx* ctx, ast* Node);
static const type* analyzerLiteral (analyzerCtx* ctx, ast* Node);
static const type* analyzerCompoundLiteral (analyzerCtx* ctx, ast* Node);

/**
 * Returns whether the (binary) operator is one that can only act on
 * numeric types (e.g. int, char, not bool, not x*)
 */
static bool isNumericBOP (const char* o) {
    return    !strcmp(o, "+") || !strcmp(o, "-")
           || !strcmp(o, "*") || !strcmp(o, "/")
           || !strcmp(o, "%")
           || !strcmp(o, "&") || !strcmp(o, "|")
           || !strcmp(o, "^")
           || !strcmp(o, "<<") || !strcmp(o, ">>")
           || !strcmp(o, "+=") || !strcmp(o, "-=")
           || !strcmp(o, "*=") || !strcmp(o, "/=")
           || !strcmp(o, "%=")
           || !strcmp(o, "&=") || !strcmp(o, "|=")
           || !strcmp(o, "^=")
           || !strcmp(o, "<<=") || !strcmp(o, ">>=");
}

static bool isBitwiseBOP (const char* o) {
    return    !strcmp(o, "&") || !strcmp(o, "|")
           || !strcmp(o, "&=") || !strcmp(o, "|=");
}

/**
 * Is it an ordinal operator (defines an ordering...)?
 */
static bool isOrdinalBOP (const char* o) {
    return    !strcmp(o, ">") || !strcmp(o, "<")
           || !strcmp(o, ">=") || !strcmp(o, "<=");
}

static bool isEqualityBOP (const char* o) {
    return !strcmp(o, "==") || !strcmp(o, "!=");
}

static bool isAssignmentBOP (const char* o) {
    return    !strcmp(o, "=")
           || !strcmp(o, "+=") || !strcmp(o, "-=")
           || !strcmp(o, "*=") || !strcmp(o, "/=")
           || !strcmp(o, "%=")
           || !strcmp(o, "&=") || !strcmp(o, "|=")
           || !strcmp(o, "^=")
           || !strcmp(o, "<<=") || !strcmp(o, ">>=");
}

static bool isLogicalBOP (const char* o) {
    return !strcmp(o, "&&") || !strcmp(o, "||");
}

/**
 * Does this operator access struct/class members of its LHS?
 */
static bool isMemberBOP (const char* o) {
    return !strcmp(o, ".") || !strcmp(o, "->");
}

/**
 * Does this member dereference its LHS?
 */
static bool isDerefBOP (const char* o) {
    return !strcmp(o, "->");
}

/**
 * Is this the ',' operator? Yes, a bit trivial, this class
 */
static bool isCommaBOP (const char* o) {
    return !strcmp(o, ",");
}

static bool isNodeLvalue (const ast* Node) {
    if (Node->tag == astBOP) {
        if (   isNumericBOP(Node->o) || isOrdinalBOP(Node->o)
            || isEqualityBOP(Node->o) || isLogicalBOP(Node->o))
            return false;

        else if (isMemberBOP(Node->o))
            /*if '->': lvalue (ptr dereferenced)
              if '.': lvalueness matches the struct it came from*/
            return isDerefBOP(Node->o) ? true
                                       : isNodeLvalue(Node->l);

        else if (isCommaBOP(Node->o))
            return isNodeLvalue(Node->r);

        else {
            debugErrorUnhandled("isNodeLvalue", "operator", Node->o);
            return true;
        }

    } else if (Node->tag == astUOP)
        /*lvalue iff dereference*/
        return !strcmp(Node->o, "*");

    else if (Node->tag == astTOP)
        /*Unify types*/
        return    isNodeLvalue(Node->l)
               && isNodeLvalue(Node->r);

    else if (Node->tag == astIndex)
        return true;

    else if (   Node->tag == astCall
             || Node->tag == astSizeof)
        return false;

    else if (Node->tag == astCast)
        return isNodeLvalue(Node->r);

    else if (Node->tag == astLiteral)
        /*Refers to an object?*/
        return    (Node->litTag == literalIdent && !typeIsFunction(Node->dt))
               || Node->litTag == literalCompound;

    else if (Node->tag == astInvalid) {
        return true;

    } else {
        debugErrorUnhandled("isNodeLvalue", "AST tag", astTagGetStr(Node->tag));
        return true;
    }
}

const type* analyzerValue (analyzerCtx* ctx, ast* Node) {
    if (Node->tag == astBOP) {
        if (isNumericBOP(Node->o) || isAssignmentBOP(Node->o))
            return analyzerBOP(ctx, Node);

        else if (isOrdinalBOP(Node->o) || isEqualityBOP(Node->o))
            return analyzerComparisonBOP(ctx, Node);

        else if (isLogicalBOP(Node->o))
            return analyzerLogicalBOP(ctx, Node);

        else if (isMemberBOP(Node->o))
            return analyzerMemberBOP(ctx, Node);

        else if (isCommaBOP(Node->o))
            return analyzerCommaBOP(ctx, Node);

        else {
            debugErrorUnhandled("analyzerValue", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astUOP)
        return analyzerUOP(ctx, Node);

    else if (Node->tag == astTOP)
        return analyzerTernary(ctx, Node);

    else if (Node->tag == astIndex)
        return analyzerIndex(ctx, Node);

    else if (Node->tag == astCall)
        return analyzerCall(ctx, Node);

    else if (Node->tag == astCast)
        return analyzerCast(ctx, Node);

    else if (Node->tag == astSizeof)
        return analyzerSizeof(ctx, Node);

    else if (Node->tag == astLiteral)
        return analyzerLiteral(ctx, Node);

    else if (Node->tag == astInvalid) {
        debugMsg("Invalid");
        return Node->dt = typeCreateInvalid();

    } else {
        debugErrorUnhandled("analyzerValue", "AST tag", astTagGetStr(Node->tag));
        return Node->dt = typeCreateInvalid();
    }
}

static const type* analyzerBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("BOP");

    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Check that the operation are allowed on the operands given*/

    if (isBitwiseBOP(Node->o)) {
        if (   !(typeIsNumeric(L) || typeIsCondition(L))
            || !(typeIsNumeric(R) || (typeIsCondition(R))))
            errorTypeExpected(ctx, !(typeIsNumeric(L) || typeIsCondition(L)) ? Node->l : Node->r,
                              Node->o, "numeric type");

    } else if (isNumericBOP(Node->o))
        if (!typeIsNumeric(L) || !typeIsNumeric(R))
            errorTypeExpected(ctx, !typeIsNumeric(L) ? Node->l : Node->r,
                              Node->o, "numeric type");

    if (isAssignmentBOP(Node->o)) {
        if (!typeIsAssignment(L) || !typeIsAssignment(R))
            errorTypeExpected(ctx, !typeIsAssignment(L) ? Node->l : Node->r,
                              Node->o, "assignable type");

        if (!isNodeLvalue(Node->l))
            errorLvalue(ctx, Node->l, Node->o);
    }

    /*Work out the type of the result*/

    if (typeIsCompatible(L, R))
        Node->dt = typeDeriveFromTwo(L, R);

    else {
        errorMismatch(ctx, Node, Node->o);
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerComparisonBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("ComparisonBOP");

    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Allowed?*/

    if (isOrdinalBOP(Node->o)) {
        if (!typeIsOrdinal(L) || !typeIsOrdinal(R))
            errorTypeExpected(ctx, !typeIsOrdinal(L) ? Node->l : Node->r,
                              Node->o, "comparable type");

    } else /*if (isEqualityBOP(Node->o))*/
        if (!typeIsEquality(L) || !typeIsEquality(R))
            errorTypeExpected(ctx, !typeIsEquality(L) ? Node->l : Node->r,
                              Node->o, "comparable type");

    if (!typeIsCompatible(L, R))
        errorMismatch(ctx, Node, Node->o);

    /*Result*/

    Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    debugLeave();

    return Node->dt;
}

static const type* analyzerLogicalBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("Logical");

    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Allowed*/

    if (!typeIsCondition(L) || !typeIsCondition(R))
        errorTypeExpected(ctx, !typeIsCondition(L) ? Node->l : Node->r,
                          Node->o, "condition");

    /*Result: bool*/
    Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    debugLeave();

    return Node->dt;
}

static const type* analyzerMemberBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("MemberBOP");

    const type* L = analyzerValue(ctx, Node->l);

    const sym* record = typeGetRecord(L);

    /*Record, or ptr to record? Irrespective of which we actually need*/
    if (!record) {
        errorTypeExpected(ctx, Node->l, Node->o,
                          isDerefBOP(Node->o) ? "structure or union pointer"
                                              : "structure or union type");
        Node->dt = typeCreateInvalid();

    } else {
        /*Right level of indirection*/

        if (isDerefBOP(Node->o)) {
            if (!typeIsPtr(L))
                errorTypeExpected(ctx, Node->l, Node->o, "pointer");

        } else
            if (!typeIsInvalid(L) && typeIsPtr(L))
                errorTypeExpected(ctx, Node->l, Node->o, "direct structure or union");

        /*Incomplete, won't find any fields*/
        if (!record->complete) {
            /*Only error if it was a pointer, otherwise the true mistake
              probably lies elsewhere and will already have errored*/
            if (typeIsPtr(L))
                errorIncompletePtr(ctx, Node->l, Node->o);

            Node->dt = typeCreateInvalid();

        /*Try to find the field inside record and get return type*/
        } else {
            Node->symbol = symChild(record, (char*) Node->r->literal);

            if (Node->symbol)
                Node->dt = typeDeepDuplicate(Node->symbol->dt);

            else {
                errorMember(ctx, Node, (char*) Node->r->literal);
                Node->dt = typeCreateInvalid();
            }
        }
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerCommaBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("CommaBOP");

    analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);
    Node->dt = typeDeepDuplicate(R);

    debugLeave();

    return Node->dt;
}

static const type* analyzerUOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("UOP");

    const type* R = analyzerValue(ctx, Node->r);

    /*Numeric operator*/
    if (   !strcmp(Node->o, "+") || !strcmp(Node->o, "-")
        || !strcmp(Node->o, "++") || !strcmp(Node->o, "--")
        || !strcmp(Node->o, "~")) {
        if (!typeIsNumeric(R)) {
            errorTypeExpected(ctx, Node->r, Node->o, "numeric type");
            Node->dt = typeCreateInvalid();

        } else {
            /*Assignment operator*/
            if (!strcmp(Node->o, "++") || !strcmp(Node->o, "--"))
                if (!isNodeLvalue(Node->r))
                    errorLvalue(ctx, Node->r, Node->o);

            Node->dt = typeDeriveFrom(R);
        }

    /*Logical negation*/
    } else if (!strcmp(Node->o, "!")) {
        if (!typeIsCondition(R))
            errorTypeExpected(ctx, Node->r, Node->o, "condition");

        Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    /*Dereferencing a pointer*/
    } else if (!strcmp(Node->o, "*")) {
        if (typeIsPtr(R)) {
            Node->dt = typeDeriveBase(R);

            if (!typeIsComplete(Node->dt))
                errorIncompletePtr(ctx, Node->r, Node->o);

        } else {
            errorTypeExpected(ctx, Node->r, Node->o, "pointer");
            Node->dt = typeCreateInvalid();
        }

    /*Referencing an lvalue*/
    } else if (!strcmp(Node->o, "&")) {
        if (!isNodeLvalue(Node->r))
            errorLvalue(ctx, Node->r, Node->o);

        Node->dt = typeDerivePtr(R);

    } else {
        debugErrorUnhandled("analyzerUOP", "operator", Node->o);
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerTernary (analyzerCtx* ctx, ast* Node) {
    debugEnter("Ternary");

    const type* Cond = analyzerValue(ctx, Node->firstChild);
    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Operation allowed*/

    if (!typeIsCondition(Cond))
        errorTypeExpected(ctx, Node->firstChild, "ternary ?:", "condition value");

    /*Result types match => return type*/

    if (typeIsCompatible(L, R))
        Node->dt = typeDeriveUnified(L, R);

    else {
        errorMismatch(ctx, Node, "ternary ?:");
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerIndex (analyzerCtx* ctx, ast* Node) {
    debugEnter("Index");

    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    if (!typeIsNumeric(R))
        errorTypeExpected(ctx, Node->r, "[]", "numeric index");

    if (typeIsArray(L) || typeIsPtr(L)) {
        Node->dt = typeDeriveBase(L);

        if (!typeIsComplete(Node->dt))
            errorIncompletePtr(ctx, Node->r, "[]");

    } else {
        errorTypeExpected(ctx, Node->l, "[]", "array or pointer");
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerCall (analyzerCtx* ctx, ast* Node) {
    debugEnter("Call");

    const type* L = analyzerValue(ctx, Node->l);

    /*Find a typeFunction, if possible*/
    const type* fn = typeGetCallable(L);

    if (typeIsInvalid(L))
        Node->dt = typeCreateInvalid();

    else if (!fn) {
        errorTypeExpected(ctx, Node->l, "()", "function");
        Node->dt = typeCreateInvalid();

    } else {
        Node->dt = fn->returnType;

        /*Right number of params?*/
        if (fn->variadic ? fn->params > Node->children :
                           fn->params != Node->children)
            errorDegree(ctx, Node, "parameter", fn->params, Node->children,
                        Node->l->symbol ? Node->l->symbol->ident : "function");

        /*Do the parameter types match?*/
        else {
            ast* Current;
            int n;

            /*Traverse down the node list and params array at the same
              time, checking types*/
            for (Current = Node->firstChild, n = 0;
                 Current && n < fn->params;
                 Current = Current->nextSibling, n++) {
                const type* Param = analyzerValue(ctx, Current);

                if (!typeIsCompatible(Param, fn->paramTypes[n]))
                    errorParamMismatch(ctx, Current, Node, n, fn->paramTypes[n], Param);
            }

            /*Analyze the rest of the given params even if there were
              fewer params in the prototype (as in a variadic fn).*/
            for (; Current; Current = Current->nextSibling)
                analyzerValue(ctx, Current);
        }
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerCast (analyzerCtx* ctx, ast* Node) {
    debugEnter("Cast");

    const type* L = analyzerType(ctx, Node->l);
    /*const type* R =*/ analyzerValue(ctx, Node->r);

    /*TODO: Verify compatibility. What exactly are the rules? All numerics
            cast to each other and nothing more?*/

    Node->dt = typeDeepDuplicate(L);

    debugLeave();

    return Node->dt;
}

static const type* analyzerSizeof (analyzerCtx* ctx, ast* Node) {
    debugEnter("Sizeof");

    /*Hand it off to the relevant function, but there's no analysis
      to be done here*/

    if (Node->r->tag == astType)
        analyzerType(ctx, Node->r);

    else
        analyzerValue(ctx, Node->r);

    Node->dt = typeCreateBasic(ctx->types[builtinInt]);

    debugLeave();

    return Node->dt;
}

static const type* analyzerLiteral (analyzerCtx* ctx, ast* Node) {
    debugEnter("Literal");

    if (Node->litTag == literalInt)
        Node->dt = typeCreateBasic(ctx->types[builtinInt]);

    else if (Node->litTag == literalChar)
        Node->dt = typeCreateBasic(ctx->types[builtinChar]);

    else if (Node->litTag == literalBool)
        Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    else if (Node->litTag == literalStr)
        /* char* */
        Node->dt = typeCreatePtr(typeCreateBasic(ctx->types[builtinChar]));

    else if (Node->litTag == literalIdent) {
        if (Node->symbol->tag == symEnumConstant || Node->symbol->tag == symId || Node->symbol->tag == symParam) {
            if (Node->symbol->dt)
                Node->dt = typeDeepDuplicate(Node->symbol->dt);

            else {
                debugError("analyzerLiteral", "Symbol '%s' referenced without type", Node->symbol->ident);
                Node->dt = typeCreateInvalid();
            }

        } else {
            errorIllegalSymAsValue(ctx, Node, Node->symbol);
            Node->dt = typeCreateInvalid();
        }

    } else if (Node->litTag == literalCompound)
        analyzerCompoundLiteral(ctx, Node);

    else if (Node->litTag == literalInit) {
        errorCompoundLiteralWithoutType(ctx, Node);
        Node->dt = typeCreateInvalid();

    } else {
        debugErrorUnhandled("analyzerLiteral", "literal tag", literalTagGetStr(Node->litTag));
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static const type* analyzerCompoundLiteral (analyzerCtx* ctx, ast* Node) {
    debugEnter("CompoundLiteral");

    analyzerInitOrCompoundLiteral(ctx, Node, analyzerType(ctx, Node->l), false);
    Node->symbol->dt = typeDeepDuplicate(Node->dt);

    debugLeave();

    return Node->dt;
}

const type* analyzerInitOrCompoundLiteral (analyzerCtx* ctx, ast* Node, const type* DT, bool directInit) {
    debugEnter("InitOrCompoundtLiteral");

    Node->dt = typeDeepDuplicate(DT);

    if (typeIsInvalid(DT))
        ;

    /*struct: check each field in order, check lengths match*/
    else if (typeIsStruct(DT)) {
        const sym* structSym = DT->basic;

        /*Only force direct initializations (excl. compound literals) to specify
          no fields*/
        if (Node->children == 0 && !directInit)
            ;

        else if (structSym->children != Node->children)
            errorDegree(ctx, Node, "field", structSym->children, Node->children, structSym->ident);

        else {
            ast* current;
            sym* field;

            for (current = Node->firstChild, field = structSym->firstChild;
                 current && field;
                 current = current->nextSibling, field = field->nextSibling) {
                if (current->tag == astEmpty)
                    continue;

                else if (current->tag == astLiteral && current->litTag == literalInit)
                    analyzerInitOrCompoundLiteral(ctx, current, field->dt, false);

                else
                    analyzerValue(ctx, current);

                if (!typeIsCompatible(current->dt, field->dt))
                    errorInitFieldMismatch(ctx, current, structSym, field);
            }
        }

    /*Array: check that all are of the right type, complain only once*/
    } else if (typeIsArray(DT)) {
        /*Allow as many inits as elements, but not more*/
        if (DT->array >= 0 && DT->array < Node->children)
            errorDegree(ctx, Node, "element", DT->array, Node->children, "array");

        for (ast* curNode = Node->firstChild;
             curNode;
             curNode = curNode->nextSibling) {
            const type* curValue;

            if (curNode->tag == astEmpty)
                continue;

            else if (curNode->tag == astLiteral && curNode->litTag == literalInit)
                curValue = analyzerInitOrCompoundLiteral(ctx, curNode, DT->base, false);

            else
                curValue = analyzerValue(ctx, curNode);

            if (!typeIsCompatible(curValue, DT->base))
                errorTypeExpectedType(ctx, curNode, "array initialization", DT->base);
        }

    /*Scalar*/
    } else {
        if (Node->children == 0 && !directInit)
            ;

        else if (Node->children != 1)
            errorDegree(ctx, Node, "element", 1, Node->children, "scalar");

        else {
            const type* R = analyzerValue(ctx, Node->firstChild);

            if (!typeIsCompatible(R, DT))
                errorTypeExpectedType(ctx, Node->r, "variable initialization", DT);
        }
    }

    debugLeave();

    return Node->dt;
}
