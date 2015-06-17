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

using "../inc/debug.h";
using "../inc/type.h";
using "../inc/ast.h";
using "../inc/sym.h";
using "../inc/error.h";

using "../inc/eval.h";

using "../inc/analyzer.h";
using "../inc/analyzer-value.h";

using "stdlib.h";

static storageTag analyzerStorage (analyzerCtx* ctx, ast* Node);

/**
 * Handle any basic node produced by parserDeclBasic.
 *
 * The given node takes ownership of the returned type.
 */
static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node);

static void analyzerStruct (analyzerCtx* ctx, ast* Node);
static void analyzerUnion (analyzerCtx* ctx, ast* Node);
static void analyzerEnum (analyzerCtx* ctx, ast* Node);

/**
 * Handle any node produced by parserDeclExpr using the following fns.
 *
 * Takes ownership of base and assigns it to the symbol.
 */
static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage);

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage);
static const type* analyzerConst (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage);
static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage);
static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, type* returnType, bool module, storageTag storage);
static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage);
static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage);

void analyzerDecl (analyzerCtx* ctx, ast* Node, bool module) {
    debugEnter("Decl");

    /*The storage qualifier given in /this/ declaration, if any*/
    const storageTag storage = analyzerStorage(ctx, Node->r);

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        const type* R = analyzerDeclNode(ctx, Current, typeDeepDuplicate(BasicDT), module, storage);

        /*Complete? Avoid complaining about typedefs and the like
          (they don't need to be complete)*/
        if (   Current->symbol && Current->symbol->tag == symId
            && !typeIsComplete(R))
            errorIncompleteDecl(ctx, Current, R);
    }

    debugLeave();
}

static storageTag analyzerStorage (analyzerCtx* ctx, ast* Node) {
    (void) ctx;

    if (!Node)
        ;

    else if (Node->tag == astMarker) {
        if (Node->marker == markerAuto)
            return storageAuto;

        else if (Node->marker == markerStatic)
            return storageStatic;

        else if (Node->marker == markerExtern)
            return storageExtern;

        else
            debugErrorUnhandledInt("analyzerStorage", "AST marker tag", Node->marker);

    } else
        debugErrorUnhandled("analyzerStorage", "AST tag", astTagGetStr(Node->tag));

    return storageUndefined;
}

const type* analyzerType (analyzerCtx* ctx, ast* Node) {
    debugEnter("Type");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, typeDeepDuplicate(BasicDT), false, storageUndefined));

    debugLeave();

    return Node->dt;
}

type* analyzerParamList (analyzerCtx* ctx, ast* Node, type* returnType) {
    debugEnter("ParamList");

    bool variadic = false;
    type** paramTypes = calloc(Node->children, sizeof(type*));
    int paramNo = 0;

    for (ast* param = Node->firstChild;
         param;
         param = param->nextSibling) {
        /*Ellipsis to indicate variadic function. The grammar has already
          ensured there is only one and that it is the final parameter.*/
        if (param->tag == astEllipsis) {
            variadic = true;
            debugMsg("Ellipsis");

        } else if (param->tag == astParam) {
            const type* BasicDT = analyzerDeclBasic(ctx, param->l);
            const type* paramType = analyzerDeclNode(ctx, param->r, typeDeepDuplicate(BasicDT), false, storageUndefined);

            paramTypes[paramNo++] = typeDeepDuplicate(paramType);

            if (!typeIsComplete(paramType))
                errorIncompleteParamDecl(ctx, param, Node, paramNo, paramType);

        } else
            debugErrorUnhandled("analyzerParamList", "AST tag", astTagGetStr(param->tag));
    }

    type* DT = typeCreateFunction(returnType, paramTypes, paramNo, variadic);

    debugLeave();

    return DT;
}

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node) {
    debugEnter(astTagGetStr(Node->tag));

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

static bool analyzerStructAnyDeclConst (ast* Node) {
    for (ast* field = Node->firstChild;
         field;
         field = field->nextSibling)
        if (   field->symbol && field->symbol->tag == symId && field->symbol->dt
            && typeIsMutable(field->symbol->dt) != mutMutable)
            return true;

    return false;
}

static void analyzerStruct (analyzerCtx* ctx, ast* Node) {
    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current, false);

        if (   !Node->symbol->hasConstFields
            && analyzerStructAnyDeclConst(Current))
            Node->symbol->hasConstFields = true;
    }

    Node->dt = typeCreateBasic(Node->symbol);

    /*TODO: check compatiblity
            of? redecls or something?*/
}

static void analyzerUnion (analyzerCtx* ctx, ast* Node) {
    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current, false);
    }

    Node->dt = typeCreateBasic(Node->symbol);
}

static void analyzerEnum (analyzerCtx* ctx, ast* Node) {
    /*Assign types and values to the constants*/

    Node->dt = typeCreateBasic(Node->symbol);

    int nextConst = 0;

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        /*Explicit assignment*/
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

        /*Assign type and value, increment nextConst*/
        if (Current->symbol) {
            analyzerDeclIdentLiteral(ctx, Current, typeDeepDuplicate(Node->dt), false, storageUndefined);
            Current->symbol->constValue = nextConst++;
        }
    }
}

static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage) {
    debugEnter(astTagGetStr(Node->tag));

    const type* DT = 0;

    if (Node->tag == astInvalid || Node->tag == astEmpty)
        ;

    else if (Node->tag == astBOP) {
        if (Node->o == opAssign)
            DT = analyzerDeclAssignBOP(ctx, Node, base, module, storage);

        else
            debugErrorUnhandled("analyzerDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astUOP) {
        if (Node->o == opDeref)
            DT = analyzerDeclPtrUOP(ctx, Node, base, module, storage);

        else
            debugErrorUnhandled("analyzerDeclNode", "operator", opTagGetStr(Node->o));

    } else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            DT = analyzerDeclIdentLiteral(ctx, Node, base, module, storage);

        else
            debugErrorUnhandled("analyzerDeclNode", "literal tag", literalTagGetStr(Node->litTag));

    } else if (Node->tag == astConst)
        DT = analyzerConst(ctx, Node, base, module, storage);

    else if (Node->tag == astCall)
        DT = analyzerDeclCall(ctx, Node, base, module, storage);

    else if (Node->tag == astIndex)
        DT = analyzerDeclIndex(ctx, Node, base, module, storage);

    else
        debugErrorUnhandled("analyzerDeclNode", "AST tag", astTagGetStr(Node->tag));

    /*Assign type for error states*/

    if (Node->symbol && !Node->symbol->dt)
        Node->symbol->dt = base;

    else if (!DT)
        Node->dt = base;

    if (!DT)
        DT = base;

    debugLeave();

    return DT;
}

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage) {
    /*Verify types*/

    const type* L = analyzerDeclNode(ctx, Node->l, base, module, storage);

    /*Struct/array initializer?*/
    if (Node->r->tag == astLiteral && Node->r->litTag == literalInit) {
        analyzerCompoundInit(ctx, Node->r, L);

        if (typeIsArray(L) && typeGetArraySize(L) < 0)
            typeSetArraySize(Node->symbol->dt, typeGetArraySize(Node->r->dt));

    /*Regular assignment*/
    } else {
        const type* R = analyzerValue(ctx, Node->r);

        if (!typeIsCompatible(R, L))
            errorInitMismatch(ctx, Node->l, Node->r);

        else if (!typeIsAssignment(L))
            errorOpTypeExpected(ctx, Node->l, Node->o, "assignable type");
    }

    /*Is an assignment to this symbol valid?*/

    if (Node->l->symbol->tag == symTypedef)
        errorIllegalInit(ctx, Node, "a typedef");

    /*The initialization is illegal if this decl is extern,
      irrespective of whether the symbol has previously been
      declared extern*/
    else if (storage == storageExtern)
        errorIllegalInit(ctx, Node, "an extern variable");

    /*If this symbol is statically stored (implicitly, or by a
      previous decl) require a constant initializer*/
    else if (Node->l->symbol->storage == storageStatic) {
        if (evalIsConstantInit(Node->r))
            errorStaticCompileTimeKnown(ctx, Node->r, Node->l->symbol);
    }

    return L;
}

static const type* analyzerConst (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage) {
    if (base->qual.isConst)
        errorAlreadyConst(ctx, Node);

    else if (typeIsArray(base) || typeIsFunction(base))
        errorIllegalConst(ctx, Node, base);

    else
        base->qual.isConst = true;

    return analyzerDeclNode(ctx, Node->r, base, module, storage);
}

static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage) {
    return analyzerDeclNode(ctx, Node->r, typeCreatePtr(base), module, storage);
}

static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, type* returnType, bool module, storageTag storage) {
    if (!typeIsComplete(returnType))
        errorIncompleteReturnDecl(ctx, Node, returnType);

    type* DT = analyzerParamList(ctx, Node, returnType);
    return analyzerDeclNode(ctx, Node->l, DT, module, storage);
}

static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage) {
    int size = arraySizeError;

    /*Unspecified size, we will (hopefully) infer it from an initializer
      [] as a synonym for * was earlier differentiated by the parser*/
    if (Node->r->tag == astEmpty)
        size = arraySizeUnspecified;

    else {
        /*Get the size*/
        analyzerValue(ctx, Node->r);
        evalResult sizeExpr = eval(ctx->arch, Node->r);

        /*Validate it*/

        if (!sizeExpr.known)
            errorCompileTimeKnown(ctx, Node->r, Node->l->symbol, "array size");

        else if (sizeExpr.value <= 0)
            errorIllegalArraySize(ctx, Node->r, Node->l->symbol, sizeExpr.value);

        else
            size = sizeExpr.value;
    }

    return analyzerDeclNode(ctx, Node->l, typeCreateArray(base, size), module, storage);
}

static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, type* base, bool module, storageTag storage) {
    bool fn = typeIsFunction(base);
    /*The storage tag defaults to:
        - extern if the symbol is a function,
        - static if a module level variable,
        - auto otherwise,
     But an explicit storage specifier overrides all.*/
    Node->storage =   storage ? storage
                    : fn ? storageExtern
                    : module ? storageStatic : storageAuto;

    if (Node->symbol) {
        if (Node->symbol->tag == symId && !Node->symbol->storage) {
            /*Assign the storage tag*/
            Node->symbol->storage = Node->storage;
        }

        if (   Node->symbol->tag == symId
            || Node->symbol->tag == symParam
            || Node->symbol->tag == symEnumConstant
            || Node->symbol->tag == symTypedef) {
            /*Assign the type*/
            if (!Node->symbol->dt)
                Node->symbol->dt = base;

            /*Don't bother checking types if param
              Any conflicts will be reported by the function itself*/
            else if (Node->symbol->tag == symParam)
                ;

            /*Not the first declaration of this symbol, check type matches*/
            else if (!typeIsEqual(Node->symbol->dt, base))
                errorConflictingDeclarations(ctx, Node, Node->symbol, base);

            /*Even if types match, not allowed to be redeclared if a variable*/
            else if (   Node->symbol->tag == symId && !fn
                     && Node->symbol->parent->tag != symStruct
                     && Node->symbol->parent->tag != symUnion
                     && Node->symbol->storage != storageExtern)
                errorRedeclared(ctx, Node, Node->symbol);

            reportSymbol(Node->symbol);
        }

    } else
        reportType(base);

    /*Take ownership of the base if unable to give it to a symbol*/
    if (!Node->symbol || Node->symbol->dt != base)
        Node->dt = base;

    return base;
}
