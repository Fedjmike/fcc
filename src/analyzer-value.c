#include "../inc/analyzer-value.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/error.h"

#include "../inc/compiler.h"

#include "../inc/eval.h"
#include "../inc/analyzer.h"
#include "../inc/analyzer-decl.h"

#include "stdlib.h"

static void analyzerBOP (analyzerCtx* ctx, ast* Node);
static void analyzerComparisonBOP (analyzerCtx* ctx, ast* Node);
static void analyzerLogicalBOP (analyzerCtx* ctx, ast* Node);
static void analyzerMemberBOP (analyzerCtx* ctx, ast* Node);
static void analyzerCommaBOP (analyzerCtx* ctx, ast* Node);

static void analyzerUOP (analyzerCtx* ctx, ast* Node);
static void analyzerTernary (analyzerCtx* ctx, ast* Node);
static void analyzerIndex (analyzerCtx* ctx, ast* Node);
static void analyzerCall (analyzerCtx* ctx, ast* Node);
static void analyzerCast (analyzerCtx* ctx, ast* Node);
static void analyzerSizeof (analyzerCtx* ctx, ast* Node);
static void analyzerLiteral (analyzerCtx* ctx, ast* Node);
static void analyzerCompoundLiteral (analyzerCtx* ctx, ast* Node);
static void analyzerStructInit (analyzerCtx* ctx, ast* Node, const type* DT);
static void analyzerArrayInit (analyzerCtx* ctx, ast* Node, const type* DT);
static void analyzerElementInit (analyzerCtx* ctx, ast* Node, const type* expected);
static void analyzerLambda (analyzerCtx* ctx, ast* Node);

static void analyzerVAStart (analyzerCtx* ctx, ast* Node);
static void analyzerVAEnd (analyzerCtx* ctx, ast* Node);
static void analyzerVAArg (analyzerCtx* ctx, ast* Node);
static void analyzerVACopy (analyzerCtx* ctx, ast* Node);
static void analyzerVAListParam (analyzerCtx* ctx, ast* Node, const char* where, const char* which);

static bool isNodeLvalue (const ast* Node) {
    if (Node->tag == astBOP) {
        if (   opIsNumeric(Node->o) || opIsOrdinal(Node->o)
            || opIsEquality(Node->o) || opIsLogical(Node->o))
            return false;

        else if (opIsMember(Node->o))
            /*if '->': lvalue (ptr dereferenced)
              if '.': lvalueness matches the struct it came from*/
            return opIsDeref(Node->o) ? true
                                      : isNodeLvalue(Node->l);

        else if (Node->o == opComma)
            return isNodeLvalue(Node->r);

        else {
            debugErrorUnhandled("isNodeLvalue", "operator", opTagGetStr(Node->o));
            return true;
        }

    } else if (Node->tag == astUOP)
        return Node->o == opDeref;

    else if (Node->tag == astTOP)
        /*Unify types*/
        return    isNodeLvalue(Node->l)
               && isNodeLvalue(Node->r);

    else if (Node->tag == astIndex)
        return true;

    else if (   Node->tag == astCall || Node->tag == astCast
             || Node->tag == astVAStart || Node->tag == astVAEnd
             || Node->tag == astVAArg || Node->tag == astVACopy
             || Node->tag == astSizeof )
        return false;

    else if (Node->tag == astLiteral)
        /*Refers to an object?*/
        return    (   Node->litTag == literalIdent
                   && (Node->symbol && (   Node->symbol->tag == symId
                                        || Node->symbol->tag == symParam))
                   && !typeIsFunction(Node->dt))
               || Node->litTag == literalCompound
               || typeIsInvalid(Node->dt);

    else if (Node->tag == astInvalid)
        return true;

    else {
        debugErrorUnhandled("isNodeLvalue", "AST tag", astTagGetStr(Node->tag));
        return true;
    }
}

static sym* analyzerRecordMember (analyzerCtx* ctx, ast* Node, opTag o, const sym* record) {
    Node->symbol = symChild(record, (char*) Node->literal);

    if (!Node->symbol)
        errorMember(ctx, Node, o, (char*) Node->literal);

    return Node->symbol;
}

const type* analyzerValue (analyzerCtx* ctx, ast* Node) {
    debugEnter(astTagGetStr(Node->tag));

    if (Node->tag == astBOP) {
        if (opIsNumeric(Node->o) || opIsAssignment(Node->o))
            analyzerBOP(ctx, Node);

        else if (opIsOrdinal(Node->o) || opIsEquality(Node->o))
            analyzerComparisonBOP(ctx, Node);

        else if (opIsLogical(Node->o))
            analyzerLogicalBOP(ctx, Node);

        else if (opIsMember(Node->o))
            analyzerMemberBOP(ctx, Node);

        else if (Node->o == opComma)
            analyzerCommaBOP(ctx, Node);

        else {
            debugErrorUnhandled("analyzerValue", "operator", opTagGetStr(Node->o));
            Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astUOP)
        analyzerUOP(ctx, Node);

    else if (Node->tag == astTOP)
        analyzerTernary(ctx, Node);

    else if (Node->tag == astIndex)
        analyzerIndex(ctx, Node);

    else if (Node->tag == astCall)
        analyzerCall(ctx, Node);

    else if (Node->tag == astCast)
        analyzerCast(ctx, Node);

    else if (Node->tag == astSizeof)
        analyzerSizeof(ctx, Node);

    else if (Node->tag == astLiteral)
        analyzerLiteral(ctx, Node);

    else if (Node->tag == astVAStart)
        analyzerVAStart(ctx, Node);

    else if (Node->tag == astVAEnd)
        analyzerVAEnd(ctx, Node);

    else if (Node->tag == astVAArg)
        analyzerVAArg(ctx, Node);

    else if (Node->tag == astVACopy)
        analyzerVACopy(ctx, Node);

    else if (Node->tag == astInvalid)
        Node->dt = typeCreateInvalid();

    else {
        debugErrorUnhandled("analyzerValue", "AST tag", astTagGetStr(Node->tag));
        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static void analyzerBOP (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Check that the operation are allowed on the operands given*/

    if (opIsBitwise(Node->o)) {
        if (   !(typeIsNumeric(L) || typeIsCondition(L))
            || !(typeIsNumeric(R) || (typeIsCondition(R))))
            errorOpTypeExpected(ctx, !(typeIsNumeric(L) || typeIsCondition(L)) ? Node->l : Node->r,
                                Node->o, "numeric type");

    } else {
        if (opIsNumeric(Node->o))
            if (!typeIsNumeric(L) || !typeIsNumeric(R))
                errorOpTypeExpected(ctx, !typeIsNumeric(L) ? Node->l : Node->r,
                                    Node->o, "numeric type");
    }

    if (opIsAssignment(Node->o)) {
        if (!typeIsAssignment(L))
            errorOpTypeExpected(ctx, Node->l, Node->o, "assignable type");

        if (!isNodeLvalue(Node->l))
            errorLvalue(ctx, Node->l, Node->o);

        else if (!typeIsMutable(L))
            errorConstAssignment(ctx, Node->l, Node->o);
    }

    /*Work out the type of the result*/

    if (typeIsCompatible(L, R))
        Node->dt = typeDeriveFromTwo(L, R);

    else {
        errorMismatch(ctx, Node, Node->o);
        Node->dt = typeCreateInvalid();
    }
}

static void analyzerComparisonBOP (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Allowed?*/

    if (opIsOrdinal(Node->o)) {
        if (!typeIsOrdinal(L) || !typeIsOrdinal(R))
            errorOpTypeExpected(ctx, !typeIsOrdinal(L) ? Node->l : Node->r,
                                Node->o, "comparable type");

    } else /*if (opIsEquality(Node->o))*/
        if (!typeIsEquality(L) || !typeIsEquality(R))
            errorOpTypeExpected(ctx, !typeIsEquality(L) ? Node->l : Node->r,
                                Node->o, "comparable type");

    if (!typeIsCompatible(L, R))
        errorMismatch(ctx, Node, Node->o);

    /*Result*/

    Node->dt = typeCreateBasic(ctx->types[builtinBool]);
}

static void analyzerLogicalBOP (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Allowed*/

    if (!typeIsCondition(L) || !typeIsCondition(R))
        errorOpTypeExpected(ctx, !typeIsCondition(L) ? Node->l : Node->r,
                            Node->o, "condition");

    /*Result: bool*/
    Node->dt = typeCreateBasic(ctx->types[builtinBool]);
}

static void analyzerMemberBOP (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerValue(ctx, Node->l);

    const sym* record = typeGetRecord(L);

    if (typeIsInvalid(L) || typeIsInvalid(typeGetBase(L)))
        Node->dt = typeCreateInvalid();

    /*Record, or ptr to record? Irrespective of which we actually need*/
    else if (!record) {
        errorOpTypeExpected(ctx, Node->l, Node->o,
                            opIsDeref(Node->o) ? "structure or union pointer"
                                               : "structure or union type");
        Node->dt = typeCreateInvalid();

    } else {
        /*Right level of indirection*/

        if (opIsDeref(Node->o)) {
            if (!typeIsPtr(L))
                errorOpTypeExpected(ctx, Node->l, Node->o, "pointer");

        } else
            if (!typeIsInvalid(L) && typeIsPtr(L))
                errorOpTypeExpected(ctx, Node->l, Node->o, "direct structure or union");

        /*Incomplete, won't find any fields*/
        if (!record->complete) {
            /*Only give an error if it was a pointer, otherwise the true mistake
              probably lies in the declaration and will already have errored*/
            if (typeIsPtr(L))
                errorIncompletePtr(ctx, Node->l, Node->o);

            Node->dt = typeCreateInvalid();

        /*Try to find the field inside the record and get the return type*/
        } else {
            Node->symbol = analyzerRecordMember(ctx, Node->r, Node->o, record);
            Node->dt = Node->symbol ? typeDeepDuplicate(Node->symbol->dt)
                                    : typeCreateInvalid();
        }
    }
}

static void analyzerCommaBOP (analyzerCtx* ctx, ast* Node) {
    analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);
    Node->dt = typeDeepDuplicate(R);
}

static void analyzerUOP (analyzerCtx* ctx, ast* Node) {
    const type* R = analyzerValue(ctx, Node->r);

    /*Numeric operator*/
    if (   Node->o == opUnaryPlus || Node->o == opNegate
        || Node->o == opPostIncrement || Node->o == opPostDecrement
        || Node->o == opPreIncrement || Node->o == opPreDecrement
        || Node->o == opBitwiseNot) {
        if (!typeIsNumeric(R)) {
            errorOpTypeExpected(ctx, Node->r, Node->o, "numeric type");
            Node->dt = typeCreateInvalid();

        } else {
            /*Assignment operator*/
            if (   Node->o == opPostIncrement || Node->o == opPostDecrement
                || Node->o == opPreIncrement || Node->o == opPreDecrement) {
                if (!isNodeLvalue(Node->r))
                    errorLvalue(ctx, Node->r, Node->o);

                else if (!typeIsMutable(R))
                    errorConstAssignment(ctx, Node->r, Node->o);
            }

            Node->dt = typeDeriveFrom(R);
        }

    /*Logical negation*/
    } else if (Node->o == opLogicalNot) {
        if (!typeIsCondition(R))
            errorOpTypeExpected(ctx, Node->r, Node->o, "condition");

        Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    /*Dereferencing a pointer*/
    } else if (Node->o == opDeref) {
        if (typeIsPtr(R)) {
            Node->dt = typeDeriveBase(R);

            if (!typeIsComplete(Node->dt))
                errorIncompletePtr(ctx, Node->r, Node->o);

            else if (!typeIsNonVoid(Node->dt))
                errorVoidDeref(ctx, Node->r, Node->o);

        } else {
            errorOpTypeExpected(ctx, Node->r, Node->o, "pointer");
            Node->dt = typeCreateInvalid();
        }

    /*Referencing an lvalue*/
    } else if (Node->o == opAddressOf) {
        if (!isNodeLvalue(Node->r))
            errorLvalue(ctx, Node->r, Node->o);

        Node->dt = typeDerivePtr(R);

    } else {
        debugErrorUnhandled("analyzerUOP", "operator", opTagGetStr(Node->o));
        Node->dt = typeCreateInvalid();
    }
}

static void analyzerTernary (analyzerCtx* ctx, ast* Node) {
    const type* Cond = analyzerValue(ctx, Node->firstChild);
    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    /*Operation allowed*/

    if (!typeIsCondition(Cond))
        errorOpTypeExpected(ctx, Node->firstChild, opTernary, "condition value");

    /*Result types match => return type*/

    if (typeIsCompatible(L, R))
        Node->dt = typeDeriveUnified(L, R);

    else {
        errorMismatch(ctx, Node, opTernary);
        Node->dt = typeCreateInvalid();
    }
}

static void analyzerIndex (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerValue(ctx, Node->l);
    const type* R = analyzerValue(ctx, Node->r);

    if (!typeIsNumeric(R))
        errorOpTypeExpected(ctx, Node->r, opIndex, "numeric index");

    if (typeIsArray(L) || typeIsPtr(L)) {
        Node->dt = typeDeriveBase(L);

        if (!typeIsComplete(Node->dt))
            errorIncompletePtr(ctx, Node->l, opIndex);

        else if (!typeIsNonVoid(Node->dt))
            errorVoidDeref(ctx, Node->l, opIndex);

    } else {
        errorOpTypeExpected(ctx, Node->l, opIndex, "array or pointer");
        Node->dt = typeCreateInvalid();
    }
}

static void analyzerCall (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerValue(ctx, Node->l);

    /*Find a typeFunction, if possible*/
    const type* fn = typeGetCallable(L);

    if (typeIsInvalid(L) || !fn) {
        if (!typeIsInvalid(L))
            errorOpTypeExpected(ctx, Node->l, opCall, "function");

        Node->dt = typeCreateInvalid();

        for (ast* param = Node->firstChild; param; param = param->nextSibling)
            analyzerValue(ctx, param);

    } else {
        Node->dt = typeDeriveReturn(fn);

        /*Right number of params?*/
        if (fn->variadic ? fn->params > Node->children :
                           fn->params != Node->children)
            errorDegree(ctx, Node, "parameters", fn->params, Node->children,
                        Node->l->symbol ? Node->l->symbol->ident : "function");

        /*Do the parameter types match?*/
        else {
            ast* param;
            int n;

            /*Traverse down the node list and params array at the same
              time, checking types*/
            for (param = Node->firstChild, n = 0;
                 param && n < fn->params;
                 param = param->nextSibling, n++) {
                const type* Param = analyzerValue(ctx, param);

                if (!typeIsCompatible(Param, fn->paramTypes[n]))
                    errorParamMismatch(ctx, param, Node->l, n, fn->paramTypes[n], Param);
            }

            /*Analyze the rest of the given params even if there were
              fewer params in the prototype (as in a variadic fn).*/
            for (; param; param = param->nextSibling)
                analyzerValue(ctx, param);
        }
    }
}

static void analyzerCast (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerType(ctx, Node->l);
    /*const type* R =*/ analyzerValue(ctx, Node->r);

    /*TODO: Verify compatibility. What exactly are the rules? All numerics
            cast to each other and nothing more?*/

    Node->dt = typeDeepDuplicate(L);
}

static void analyzerSizeof (analyzerCtx* ctx, ast* Node) {
    /*Hand it off to the relevant function, but there's no analysis
      to be done here*/

    if (Node->r->tag == astType)
        analyzerType(ctx, Node->r);

    else
        analyzerValue(ctx, Node->r);

    Node->dt = typeCreateBasic(ctx->types[builtinInt]);
}

static void analyzerLiteral (analyzerCtx* ctx, ast* Node) {
    if (Node->litTag == literalInt)
        Node->dt = typeCreateBasic(ctx->types[builtinInt]);

    else if (Node->litTag == literalChar)
        Node->dt = typeCreateBasic(ctx->types[builtinChar]);

    else if (Node->litTag == literalBool)
        Node->dt = typeCreateBasic(ctx->types[builtinBool]);

    else if (Node->litTag == literalStr) {
        /* const char* */
        Node->dt = typeCreatePtr(typeCreateBasic(ctx->types[builtinChar]));
        Node->dt->base->qual.isConst = true;

    } else if (Node->litTag == literalIdent) {
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

    } else if (Node->litTag == literalCompound) {
        analyzerCompoundLiteral(ctx, Node);

    } else if (Node->litTag == literalLambda) {
        analyzerLambda(ctx, Node);

    } else if (Node->litTag == literalInit) {
        errorCompoundLiteralWithoutType(ctx, Node);
        Node->dt = typeCreateInvalid();

    } else {
        debugErrorUnhandled("analyzerLiteral", "literal tag", literalTagGetStr(Node->litTag));
        Node->dt = typeCreateInvalid();
    }
}

static void analyzerCompoundLiteral (analyzerCtx* ctx, ast* Node) {
    const type* L = analyzerType(ctx, Node->l);

    if (!typeIsComplete(L))
        errorIncompleteCompound(ctx, Node, L);

    analyzerCompoundInit(ctx, Node, L, false);
    Node->symbol->dt = typeDeepDuplicate(Node->dt);
    Node->symbol->storage = storageAuto;
}

void analyzerCompoundInit (analyzerCtx* ctx, ast* Node, const type* DT, bool directInit) {
    Node->dt = typeDeepDuplicate(DT);

    if (typeIsInvalid(DT))
        ;

    else if (typeIsStruct(DT))
        analyzerStructInit(ctx, Node, DT);

    else if (typeIsArray(DT))
        analyzerArrayInit(ctx, Node, DT);

    /*Scalar*/
    else {
        if (Node->children == 0 && !directInit)
            ;

        else if (Node->children != 1)
            errorDegree(ctx, Node, "elements", 1, Node->children, "scalar");

        else {
            const type* R = analyzerValue(ctx, Node->firstChild);

            if (!typeIsCompatible(R, DT))
                errorTypeExpectedType(ctx, Node->r, "variable initialization", DT);
        }
    }
}

static void analyzerStructInit (analyzerCtx* ctx, ast* Node, const type* DT) {
    const sym* record = typeGetBasic(DT);

    /*If there is an error relating to designators, there will be no way to
      reliably pick fields to compare against. This flag will remain set
      until the next designator.*/
    bool error = false;

    /*Index of the next field to analyze*/
    int index = 0;

    for (ast* current = Node->firstChild;
         current;
         current = current->nextSibling) {
        ast* value = current;
        const sym* field = 0;

        /*Explicit field?*/
        if (current->tag == astMarker) {
            if (current->marker == markerStructDesignatedInit) {
                /*Try to find the field in the record, emitting an error if not present*/
                field = analyzerRecordMember(ctx, current->l, opMember, record);
                value = current->r;
                error = false;

            } else if (current->marker == markerArrayDesignatedInit) {
                errorWrongInitDesignator(ctx, current, DT);
                error = true;

            } else
                debugErrorUnhandledInt("analyzerStructElementInit", "AST marker tag", current->marker);

        /*Implied, get the next field*/
        } else if (!error) {
            field = vectorGet(&record->children, index);

            if (!field) {
                errorInitExcessFields(ctx, current, record, field);
                break;
            }
        }

        /*If a field was determined, analyze the value and compare types*/
        if (field) {
            analyzerElementInit(ctx, value, field->dt);

            if (!typeIsCompatible(value->dt, field->dt))
                errorInitFieldMismatch(ctx, value, record, field);

            index = field->nthChild+1;
        }
    }
}

static void analyzerArrayInit (analyzerCtx* ctx, ast* Node, const type* DT) {
    int elementNo = typeGetArraySize(DT);
    const type* base = typeGetBase(DT);

    bool error = false;
    int maxIndex = 0, index = 0;

    for (ast *current = Node->firstChild;
         current;
         current = current->nextSibling) {
        ast* value = current;

        /*Explicit index?*/
        if (current->tag == astMarker) {
            if (current->marker == markerArrayDesignatedInit) {
                evalResult desig = eval(ctx->arch, current->l);

                if (!desig.known)
                    errorConstantInitIndex(ctx, Node);

                index = current->l->constant = desig.value;

                value = current->r;
                error = false;

            } else if (current->marker == markerStructDesignatedInit) {
                errorWrongInitDesignator(ctx, current, DT);
                error = true;

            } else
                debugErrorUnhandledInt("analyzerArrayInit", "AST marker tag", current->marker);
        }

        if (!error) {
            analyzerElementInit(ctx, value, base);

            if (!typeIsCompatible(value->dt, base))
                errorTypeExpectedType(ctx, value, "array initialization", base);

            if (index > maxIndex)
                maxIndex = index;

            index++;
        }
    }

    if (maxIndex >= elementNo)
        errorDegree(ctx, Node, "elements", elementNo, maxIndex+1, "array");
}

static void analyzerElementInit (analyzerCtx* ctx, ast* Node, const type* expected) {
    /*Skipped initializer*/
    if (Node->tag == astEmpty)
        Node->dt = typeDeepDuplicate(expected);

    /*Recursive initialization*/
    else if (Node->tag == astLiteral && Node->litTag == literalInit)
        analyzerCompoundInit(ctx, Node, expected, false);

    /*Regular value*/
    else
        analyzerValue(ctx, Node);
}

static analyzerFnCtx analyzerPushLambda (analyzerCtx* ctx, sym* fn) {
    analyzerFnCtx old = ctx->fnctx;
    ctx->fnctx.fn = fn;
    ctx->fnctx.returnType = 0;
    return old;
}

static void analyzerLambda (analyzerCtx* ctx, ast* Node) {
    /*Param list*/
    type* fn = analyzerParamList(ctx, Node, 0);

    /*Body*/

    if (Node->r->tag == astCode) {
        analyzerFnCtx old = analyzerPushLambda(ctx, Node->symbol);
        analyzerNode(ctx, Node->r);
        /*Take ownership of the ret type which has been inferred from the code*/
        fn->returnType = ctx->fnctx.returnType ? ctx->fnctx.returnType
                                               : typeCreateBasic(ctx->types[builtinVoid]);
        ctx->fnctx = old;

    } else
        fn->returnType = typeDeepDuplicate(analyzerValue(ctx, Node->r));

    /*Result*/
    Node->dt = fn;
    Node->symbol->dt = typeDeepDuplicate(Node->dt);
    Node->symbol->storage = storageStatic;
}

static void analyzerVAStart (analyzerCtx* ctx, ast* Node) {
    analyzerVAListParam(ctx, Node->l, "va_start", "first");

    analyzerValue(ctx, Node->r);

    if (Node->r->symbol->tag != symParam)
        errorVAStartNonParam(ctx, Node->r);

    Node->dt = typeCreateBasic(ctx->types[builtinVoid]);
}

static void analyzerVAEnd (analyzerCtx* ctx, ast* Node) {
    analyzerVAListParam(ctx, Node->l, "va_end", "first");
    Node->dt = typeCreateBasic(ctx->types[builtinVoid]);
}

static void analyzerVAArg (analyzerCtx* ctx, ast* Node) {
    analyzerVAListParam(ctx, Node->l, "va_arg", "first");

    const type* R = analyzerType(ctx, Node->r);
    Node->dt = typeDeepDuplicate(R);
}

static void analyzerVACopy (analyzerCtx* ctx, ast* Node) {
    analyzerVAListParam(ctx, Node->l, "va_copy", "first");
    analyzerVAListParam(ctx, Node->r, "va_copy", "second");
    Node->dt = typeCreateBasic(ctx->types[builtinVoid]);
}

static void analyzerVAListParam (analyzerCtx* ctx, ast* Node, const char* where, const char* which) {
    const type* DT = analyzerValue(ctx, Node);

    if (typeGetBasic(DT) != ctx->types[builtinVAList])
        errorVAxList(ctx, Node, where, which);

    else if (!isNodeLvalue(Node))
        errorVAxLvalue(ctx, Node, where, which);
}
