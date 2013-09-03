#include "../inc/analyzer-value.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"

#include "../inc/analyzer.h"
#include "../inc/analyzer-decl.h"

#include "string.h"

static valueResult analyzerBOP (analyzerCtx* ctx, ast* Node);
static valueResult analyzerComparisonBOP (analyzerCtx* ctx, ast* Node);
static valueResult analyzerLogicalBOP (analyzerCtx* ctx, ast* Node);
static valueResult analyzerMemberBOP (analyzerCtx* ctx, ast* Node);
static valueResult analyzerCommaBOP (analyzerCtx* ctx, ast* Node);

static valueResult analyzerUOP (analyzerCtx* ctx, ast* Node);
static valueResult analyzerTernary (analyzerCtx* ctx, ast* Node);
static valueResult analyzerIndex (analyzerCtx* ctx, ast* Node);
static valueResult analyzerCall (analyzerCtx* ctx, ast* Node);
static valueResult analyzerCast (analyzerCtx* ctx, ast* Node);
static valueResult analyzerSizeof (analyzerCtx* ctx, ast* Node);
static valueResult analyzerLiteral (analyzerCtx* ctx, ast* Node);
static valueResult analyzerCompoundLiteral (analyzerCtx* ctx, ast* Node);
static valueResult analyzerInitOrCompoundLiteral (analyzerCtx* ctx, ast* Node, const type* DT);

/**
 * Returns whether the (binary) operator is one that can only act on
 * numeric types (e.g. int, char, not bool, not x*)
 */
static bool isNumericBOP (char* o) {
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

/**
 * Is it an ordinal operator (defines an ordering...)?
 */
static bool isOrdinalBOP (char* o) {
    return    !strcmp(o, ">") || !strcmp(o, "<")
           || !strcmp(o, ">=") || !strcmp(o, "<=");
}

static bool isEqualityBOP (char* o) {
    return !strcmp(o, "==") || !strcmp(o, "!=");
}

static bool isAssignmentBOP (char* o) {
    return    !strcmp(o, "=")
           || !strcmp(o, "+=") || !strcmp(o, "-=")
           || !strcmp(o, "*=") || !strcmp(o, "/=")
           || !strcmp(o, "%=")
           || !strcmp(o, "&=") || !strcmp(o, "|=")
           || !strcmp(o, "^=")
           || !strcmp(o, "<<=") || !strcmp(o, ">>=");
}

static bool isLogicalBOP (char* o) {
    return !strcmp(o, "&&") || !strcmp(o, "||");
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

valueResult analyzerValue (analyzerCtx* ctx, ast* Node) {
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
            return (valueResult) {Node->dt = typeCreateInvalid(), true};
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

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalCompound)
            return analyzerCompoundLiteral(ctx, Node);

        else
            return analyzerLiteral(ctx, Node);

    } else if (Node->tag == astInvalid) {
        debugMsg("Invalid");
        return (valueResult) {Node->dt = typeCreateInvalid(), true};

    } else {
        debugErrorUnhandled("analyzerValue", "AST tag", astTagGetStr(Node->tag));
        return (valueResult) {Node->dt = typeCreateInvalid(), true};
    }
}

static valueResult analyzerBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("BOP");

    valueResult L = analyzerValue(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    /*Check that the operation are allowed on the operands given*/

    if (isNumericBOP(Node->o))
        if (!typeIsNumeric(L.dt) || !typeIsNumeric(R.dt))
            errorOp(ctx, Node->o, "numeric type",
                    !typeIsNumeric(L.dt) ? Node->l : Node->r,
                    !typeIsNumeric(L.dt) ? L.dt : R.dt);

    if (isAssignmentBOP(Node->o)) {
        if (!typeIsAssignment(L.dt) || !typeIsAssignment(R.dt))
            errorOp(ctx, Node->o, "assignable type",
                    !typeIsAssignment(L.dt) ? Node->l : Node->r,
                    !typeIsAssignment(L.dt) ? L.dt : R.dt);

        if (!L.lvalue)
            errorLvalue(ctx, Node->o, Node->l);
    }

    /*Work out the type of the result*/

    if (typeIsCompatible(L.dt, R.dt))
        Node->dt = typeDeriveFromTwo(L.dt, R.dt);

    else {
        errorMismatch(ctx, Node, Node->o, L.dt, R.dt);
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return (valueResult) {Node->dt, false};
}

static valueResult analyzerComparisonBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("ComparisonBOP");

    valueResult L = analyzerValue(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    /*Allowed?*/

    if (isOrdinalBOP(Node->o)) {
        if (!typeIsOrdinal(L.dt) || !typeIsOrdinal(R.dt))
            errorOp(ctx, Node->o, "comparable type",
                    !typeIsOrdinal(L.dt) ? Node->l : Node->r,
                    !typeIsOrdinal(L.dt) ? L.dt : R.dt);

    } else /*if (isEqualityBOP(Node->o))*/
        if (!typeIsEquality(L.dt) || !typeIsEquality(R.dt))
            errorOp(ctx, Node->o, "comparable type",
                    !typeIsEquality(L.dt) ? Node->l : Node->r,
                    !typeIsEquality(L.dt) ? L.dt : R.dt);

    if (!typeIsCompatible(L.dt, R.dt)) {
        errorMismatch(ctx, Node, Node->o, L.dt, R.dt);
        Node->dt = typeCreateInvalid();
    }

    /*Result*/

    Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    debugLeave();

    return (valueResult) {Node->dt, false};
}

static valueResult analyzerLogicalBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("Logical");

    valueResult L = analyzerValue(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    /*Allowed*/

    if (!typeIsCondition(L.dt) || !typeIsCondition(R.dt))
        errorOp(ctx, Node->o, "condition",
                !typeIsCondition(L.dt) ? Node->l : Node->r,
                !typeIsCondition(L.dt) ? L.dt : R.dt);

    /*Result: bool*/
    Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    debugLeave();

    return (valueResult) {Node->dt, false};
}

static valueResult analyzerMemberBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("MemberBOP");

    valueResult L = analyzerValue(ctx, Node->l);

    if (!(   typeIsRecord(L.dt)
          || (L.dt->tag == typePtr && typeIsRecord(L.dt->base)))) {
        if (isDerefBOP(Node->o))
            errorOp(ctx, Node->o, "structure or union pointer", Node->l, L.dt);

        else
            errorOp(ctx, Node->o, "structure or union type", Node->l, L.dt);

        Node->dt = typeCreateInvalid();

    } else {
        if (isDerefBOP(Node->o) && !typeIsPtr(L.dt))
            errorOp(ctx, Node->o, "pointer", Node->l, L.dt);

        sym* recordSym = typeGetRecordSym(L.dt);

        if (recordSym) {
            Node->symbol = symChild(recordSym, (char*) Node->r->literal);

            if (Node->symbol)
                Node->dt = typeDeepDuplicate(Node->symbol->dt);

            else {
                errorMember(ctx, Node->o, Node->r, L.dt);
                Node->dt = typeCreateInvalid();
            }

        } else
            Node->dt = typeCreateInvalid();
    }

    debugLeave();

    /*if '->': lvalue (ptr dereferenced)
      if '.': lvalueness matches the struct it came from*/
    return (valueResult) {Node->dt,   isDerefBOP(Node->o)
                                    ? true
                                    : L.lvalue};
}

static valueResult analyzerCommaBOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("CommaBOP");

    analyzerValue(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    debugLeave();

    return (valueResult) {Node->dt = typeDeepDuplicate(R.dt), R.lvalue};
}

static valueResult analyzerUOP (analyzerCtx* ctx, ast* Node) {
    debugEnter("UOP");

    valueResult R = analyzerValue(ctx, Node->r);

    /*Numeric operator*/
    if (   !strcmp(Node->o, "+") || !strcmp(Node->o, "-")
        || !strcmp(Node->o, "++") || !strcmp(Node->o, "--")
        || !strcmp(Node->o, "~")) {
        if (!typeIsNumeric(R.dt)) {
            errorOp(ctx, Node->o, "numeric type", Node->r, R.dt);
            Node->dt = typeCreateInvalid();

        } else {
            /*Assignment operator*/
            if (!strcmp(Node->o, "++") || !strcmp(Node->o, "--"))
                if (!R.lvalue)
                    errorLvalue(ctx, Node->o, Node->r);

            Node->dt = typeDeriveFrom(R.dt);
        }

    /*Logical negation*/
    } else if (!strcmp(Node->o, "!")) {
        if (!typeIsCondition(R.dt))
            errorOp(ctx, Node->o, "condition", Node->r, R.dt);

        Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    /*Dereferencing a pointer*/
    } else if (!strcmp(Node->o, "*")) {
        if (typeIsPtr(R.dt))
            Node->dt = typeDeriveBase(R.dt);

        else {
            errorOp(ctx, Node->o, "pointer", Node->r, R.dt);
            Node->dt = typeCreateInvalid();
        }

    /*Referencing an lvalue*/
    } else if (!strcmp(Node->o, "&")) {
        if (!R.lvalue)
            errorLvalue(ctx, Node->o, Node->r);

        Node->dt = typeDerivePtr(R.dt);

    } else {
        debugErrorUnhandled("analyzerUOP", "operator", Node->o);
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    /*returns lvalue iff dereference*/
    return (valueResult) {Node->dt, !strcmp(Node->o, "*")};
}

static valueResult analyzerTernary (analyzerCtx* ctx, ast* Node) {
    debugEnter("Ternary");

    valueResult Cond = analyzerValue(ctx, Node->firstChild);
    valueResult L = analyzerValue(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    /*Operation allowed*/

    if (!typeIsCondition(Cond.dt))
        errorOp(ctx, "ternary ?:", "condition value", Node->firstChild, Cond.dt);

    /*Result types match => return type*/

    if (typeIsCompatible(L.dt, R.dt))
        Node->dt = typeDeriveUnified(L.dt, R.dt);

    else {
        errorMismatch(ctx, Node, "ternary ?:", L.dt, R.dt);
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    /*Unify types: returns an lvalue is both operands are*/
    return (valueResult) {Node->dt, L.lvalue && R.lvalue};
}

static valueResult analyzerIndex (analyzerCtx* ctx, ast* Node) {
    debugEnter("Index");

    valueResult L = analyzerValue(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    if (!typeIsNumeric(R.dt))
        errorOp(ctx, "[]", "numeric index", Node->r, R.dt);

    if (typeIsArray(L.dt) || typeIsPtr(L.dt))
        Node->dt = typeDeriveBase(L.dt);

    else {
        errorOp(ctx, "[]", "array or pointer", Node->l, L.dt);
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    /*lvalueness matches LHS*/
    return (valueResult) {Node->dt, L.lvalue};
}

static valueResult analyzerCall (analyzerCtx* ctx, ast* Node) {
    debugEnter("Call");

    valueResult L = analyzerValue(ctx, Node->l);

    /*Callable*/
    if (!typeIsCallable(L.dt)) {
        errorOp(ctx, "()", "function", Node->firstChild, L.dt);
        Node->dt = typeCreateInvalid();

    } else if (typeIsInvalid(L.dt))
        Node->dt = typeCreateInvalid();

    else {
        /*If callable, then a result type can be derived,
          regardless of parameter matches*/
        Node->dt = typeDeriveReturn(L.dt);

        const type* fn = typeIsPtr(L.dt) ? L.dt->base : L.dt;

        /*Right number of params?*/
        if (fn->variadic ? fn->params > Node->children :
                           fn->params != Node->children)
            errorDegree(ctx, Node, "parameter(s)",
                        fn->params, Node->children,
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
                valueResult Param = analyzerValue(ctx, Current);

                if (!typeIsCompatible(Param.dt, fn->paramTypes[n]))
                    errorParamMismatch(ctx, Current, n, fn->paramTypes[n], Param.dt);
            }

            /*Analyze the rest of the given params even if there were
              fewer params in the prototype (as in a variadic fn).*/
            for (; Current; Current = Current->nextSibling)
                analyzerValue(ctx, Current);
        }
    }

    debugLeave();

    return (valueResult) {Node->dt, false};
}

static valueResult analyzerCast (analyzerCtx* ctx, ast* Node) {
    debugEnter("Cast");

    const type* L = analyzerType(ctx, Node->l);
    valueResult R = analyzerValue(ctx, Node->r);

    /*TODO: Verify compatibility. What exactly are the rules? All numerics
            cast to each other and nothing more?*/

    Node->dt = typeDeepDuplicate(L);

    debugLeave();

    /*LHS's type, RHS's lvalueness*/
    return (valueResult) {Node->dt, R.lvalue};
}

static valueResult analyzerSizeof (analyzerCtx* ctx, ast* Node) {
    debugEnter("Sizeof");

    /*Hand it off to the relevant function, but there's no analysis
      to be done here*/

    if (Node->r->tag == astType)
        analyzerType(ctx, Node->r);

    else
        analyzerValue(ctx, Node->r);

    Node->dt = typeCreateBasic(ctx->types[builtinInt]);

    debugLeave();

    return (valueResult) {Node->dt, false};
}

static valueResult analyzerLiteral (analyzerCtx* ctx, ast* Node) {
    debugEnter("Literal");

    if (Node->litTag == literalInt)
        Node->dt = typeCreateBasic(ctx->types[builtinInt]);

    else if (Node->litTag == literalBool)
        Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    else if (Node->litTag == literalStr)
        /* char* */
        Node->dt = typeCreatePtr(typeCreateBasic(ctx->types[builtinChar]));

    else if (Node->litTag == literalIdent) {
        if (Node->symbol->tag == symId || Node->symbol->tag == symParam) {
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

    } else {
        debugErrorUnhandled("analyzerLiteral", "literal tag", literalTagGetStr(Node->litTag));
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return (valueResult) {Node->dt, Node->litTag == literalIdent};
}

static valueResult analyzerCompoundLiteral (analyzerCtx* ctx, ast* Node) {
    debugEnter("CompoundLiteral");

    analyzerInitOrCompoundLiteral(ctx, Node, analyzerType(ctx, Node->l));

    debugLeave();

    return (valueResult) {Node->dt, true};
}

static valueResult analyzerInitOrCompoundLiteral (analyzerCtx* ctx, ast* Node, const type* DT) {
    debugEnter("InitOrCompoundtLiteral");

    if (typeIsInvalid(DT))
        Node->dt = typeCreateInvalid();

    /*struct: check each field in order, check lengths match*/
    else if (DT->tag == typeBasic && DT->basic->tag == symStruct) {
        sym* structSym = DT->basic;

        if (structSym->children != Node->children)
            errorDegree(ctx, Node, "fields", structSym->children, Node->children, structSym->ident);

        else {
            ast* curNode;
            sym* curSym;

            for (curNode = Node->firstChild, curSym = structSym->firstChild;
                 curNode && curSym;
                 curNode = curNode->nextSibling, curSym = curSym->nextSibling) {
                valueResult curValue;

                if (curNode->tag == astLiteral && curNode->litTag == literalInit)
                    curValue = analyzerInitOrCompoundLiteral(ctx, curNode, curSym->dt);

                else
                    curValue = analyzerValue(ctx, curNode);

                if (!typeIsCompatible(curValue.dt, curSym->dt))
                    errorInitFieldMismatch(ctx, curNode, structSym, curSym, curValue.dt);
            }
        }

    /*Array: check that all are of the right type, complain only once*/
    } else if (typeIsArray(DT)) {
        if (DT->array != -1 && DT->array < Node->children)
            errorDegree(ctx, Node, "elements", DT->array, Node->children, "array");

        for (ast* curNode = Node->firstChild;
             curNode;
             curNode = curNode->nextSibling) {
            valueResult curValue;

            if (curNode->tag == astLiteral && curNode->litTag == literalInit)
                curValue = analyzerInitOrCompoundLiteral(ctx, curNode, DT->base);

            else
                curValue = analyzerValue(ctx, curNode);

            if (!typeIsCompatible(curValue.dt, DT->base))
                errorTypeExpectedType(ctx, curNode, "array initialization", DT->base, curValue.dt);
        }

    } else
        {} //TODO

    debugLeave();

    return (valueResult) {Node->dt, false};
}
