#include "../inc/type.h"

#include "../inc/debug.h"
#include "../inc/sym.h"

#include "../inc/architecture.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"

static typeQualifiers typeQualifiersCreate (void);
static type* typeCreate (typeTag tag);

/**
 * Jump through (immediate) typedefs, possibly recursively, to actual type
 */
static type* typeTryThroughTypedef (const type* DT);

/**
 * As above, but collects qualifiers, e.g.
 *
 *    typedef int x;
 *    typedef volatile x* y;
 *    typedef const y z;
 *    z a;
 * ->
 *    TryThroughTypedefQual(z) == {volatile x*, const}
 */
static type* typeTryThroughTypedefQual (const type* DT, typeQualifiers* qualOut);

static bool typeQualIsEqual (typeQualifiers L, typeQualifiers R);

static char* typeQualifiersToStr (typeQualifiers qual, const char* embedded);

/*==== Ctors/dtors ====*/

static typeQualifiers typeQualifiersCreate () {
    return (typeQualifiers) {false};
}

static type* typeCreate (typeTag tag) {
    type* DT = malloc(sizeof(type));
    DT->tag = tag;
    DT->qual.isConst = false;

    DT->basic = 0;

    DT->base = 0;
    DT->array = 0;

    DT->returnType = 0;
    DT->paramTypes = 0;
    DT->params = 0;
    DT->variadic = false;

    return DT;
}

type* typeCreateBasic (const sym* basic) {
    type* DT = typeCreate(typeBasic);
    DT->basic = basic;
    return DT;
}

type* typeCreatePtr (type* base) {
    type* DT = typeCreate(typePtr);
    DT->base = base;
    return DT;
}

type* typeCreateArray (type* base, int size) {
    type* DT = typeCreate(typeArray);
    DT->base = base;
    DT->array = size;
    return DT;
}

type* typeCreateFunction (type* returnType, type** paramTypes, int params, bool variadic) {
    type* DT = typeCreate(typeFunction);
    DT->returnType = returnType;
    DT->paramTypes = paramTypes;
    DT->params = params;
    DT->variadic = variadic;
    return DT;
}

type* typeCreateInvalid () {
    return typeCreate(typeInvalid);
}

void typeDestroy (type* DT) {
    if (DT->tag == typeBasic || DT->tag == typeInvalid)
        ;

    else if (DT->tag == typePtr || DT->tag == typeArray)
        typeDestroy(DT->base);

    else if (DT->tag == typeFunction) {
        typeDestroy(DT->returnType);

        for (int i = 0; i < DT->params; i++)
            typeDestroy(DT->paramTypes[i]);

        free(DT->paramTypes);
    }

    free(DT);
}

type* typeDeepDuplicate (const type* DT) {
    type* copy;

    if (DT->tag == typeInvalid)
        copy = typeCreateInvalid();

    else if (DT->tag == typeBasic)
        copy = typeCreateBasic(DT->basic);

    else if (DT->tag == typePtr)
        copy = typeCreatePtr(typeDeepDuplicate(DT->base));

    else if (DT->tag == typeArray)
        copy = typeCreateArray(typeDeepDuplicate(DT->base), DT->array);

    else if (DT->tag == typeFunction) {
        type** paramTypes = calloc(DT->params, sizeof(type*));

        for (int i = 0; i < DT->params; i++)
            paramTypes[i] = typeDeepDuplicate(DT->paramTypes[i]);

        copy = typeCreateFunction(typeDeepDuplicate(DT->returnType), paramTypes, DT->params, DT->variadic);

    } else {
        debugErrorUnhandled("typeDeepDuplicate", "type tag", typeTagGetStr(DT->tag));
        copy = typeCreateInvalid();
    }

    copy->qual = DT->qual;
    return copy;
}

/*==== Misc. helpers ====*/

static type* typeTryThroughTypedef (const type* DT) {
    if (DT->tag == typeBasic && DT->basic && DT->basic->tag == symTypedef)
        return typeTryThroughTypedef(DT->basic->dt);

    else
        return (type*) DT;
}

static type* typeTryThroughTypedefQual (const type* DT, typeQualifiers* qualOut) {
    if (qualOut)
        qualOut->isConst |= DT->qual.isConst;

    if (DT->tag == typeBasic && DT->basic && DT->basic->tag == symTypedef)
        return typeTryThroughTypedefQual(DT->basic->dt, qualOut);

    else
        return (type*) DT;
}

const sym* typeGetBasic (const type* DT) {
    if (!DT)
        return 0;

    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeBasic ? DT->basic : 0;
}

const type* typeGetBase (const type* DT) {
    if (!DT)
        return 0;

    DT = typeTryThroughTypedef(DT);
    return DT->tag == typePtr || DT->tag == typeArray ? DT->base : 0;
}

const type* typeGetReturn (const type* DT) {
    if (!DT)
        return 0;

    DT = typeTryThroughTypedef(DT);
    return   DT->tag == typeFunction ? DT->returnType
           : DT->tag == typePtr && typeIsFunction(DT->base) ? typeGetReturn(DT->base)
           : 0;
}

const type* typeGetRecord (const type* DT) {
    DT = typeTryThroughTypedef(DT);

    if (    DT->tag == typeBasic && DT->basic
        && (   DT->basic->tag == symStruct
            || DT->basic->tag == symUnion))
        return DT;

    else if (DT->tag == typePtr && typeIsBasic(DT->base))
        return typeGetRecord(DT->base);

    else
        return 0;
}

const type* typeGetCallable (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeFunction ? DT :
           DT->tag == typePtr && DT->base->tag == typeFunction ? DT->base : 0;
}

int typeGetArraySize (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeArray && DT->array != arraySizeError ? DT->array : 0;
}

/*==== Modification ====*/

bool typeSetArraySize (type* DT, int size) {
    DT = typeTryThroughTypedef(DT);

    if (DT->tag != typeArray)
        return false;

    DT->array = size;
    return true;
}

/*==== Derivation (deep copies with modifications) ====*/

type* typeDeriveFrom (const type* DT) {
    type* New = typeDeepDuplicate(DT);
    return New;
}

type* typeDeriveFromTwo (const type* L, const type* R) {
    if (typeIsInvalid(L))
        return typeDeepDuplicate(R);

    else if (typeIsInvalid(R))
        return typeDeepDuplicate(L);

    else {
        assert(typeIsCompatible(L, R));
        return typeDeriveFrom(L);
    }
}

type* typeDeriveUnified (const type* L, const type* R) {
    if (typeIsInvalid(L))
        return typeDeepDuplicate(R);

    else if (typeIsInvalid(R))
        return typeDeepDuplicate(L);

    else {
        assert(typeIsCompatible(L, R));

        if (typeIsEqual(L, R))
            return typeDeepDuplicate(L); //== R

        else
            return typeDeriveFromTwo(L, R);
    }
}

type* typeDeriveBase (const type* DT) {
    DT = typeTryThroughTypedef(DT);

    if (   typeIsInvalid(DT)
        || debugAssert("typeDeriveBase", "base", typeIsPtr(DT) || typeIsArray(DT)))
        return typeCreateInvalid();

    else
        return typeDeepDuplicate(DT->base);
}

type* typeDerivePtr (const type* base) {
    return typeCreatePtr(typeDeepDuplicate(base));
}

type* typeDeriveArray (const type* base, int size) {
    return typeCreateArray(typeDeepDuplicate(base), size);
}

type* typeDeriveReturn (const type* fn) {
    fn = typeGetCallable(fn);

    if (debugAssert("typeDeriveReturn", "callable param", fn != 0))
        return typeCreateInvalid();

    else
        return typeDeepDuplicate(fn->returnType);
}

/*==== Type classification ====*/

bool typeIsBasic (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeBasic || typeIsInvalid(DT);
}

bool typeIsPtr (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typePtr || typeIsInvalid(DT);
}

bool typeIsArray (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeArray || typeIsInvalid(DT);
}

bool typeIsFunction (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeFunction || typeIsInvalid(DT);
}

bool typeIsInvalid (const type* DT) {
    if (!DT)
        return false;

    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeInvalid;
}

bool typeIsComplete (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return !(   (DT->tag == typeBasic && !DT->basic->complete)
             || (DT->tag == typeArray && DT->array == arraySizeUnspecified));
}

bool typeIsVoid (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    /*Is it a built in type of size zero (void)*/
    return    (   DT->tag == typeBasic
               && DT->basic->tag == symType
               && DT->basic->size == 0)
           || typeIsInvalid(DT);
}

bool typeIsNonVoid (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (   DT->tag != typeBasic
               || DT->basic->tag != symType
               || DT->basic->size != 0)
           || typeIsInvalid(DT);
}

bool typeIsStruct (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && DT->basic->tag == symStruct)
           || typeIsInvalid(DT);
}

bool typeIsUnion (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && DT->basic->tag == symUnion)
           || typeIsInvalid(DT);
}

typeMutability typeIsMutable (const type* DT) {
    typeQualifiers qual = typeQualifiersCreate();
    DT = typeTryThroughTypedefQual(DT, &qual);

    if (qual.isConst)
        return mutConstQualified;

    else if (DT->tag == typeBasic && DT->basic->hasConstFields)
        return mutHasConstFields;

    else
        return mutMutable;
}

bool typeIsNumeric (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeNumeric))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsOrdinal (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeOrdinal))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsEquality (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeEquality))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsAssignment (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeAssignment))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsCondition (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeCondition))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

/*==== Comparisons ====*/

bool typeIsCompatible (const type* DT, const type* Model) {
    DT = typeTryThroughTypedef(DT);
    Model = typeTryThroughTypedef(Model);

    if (typeIsInvalid(DT) || typeIsInvalid(Model)) {
        return true;

    /*If function requested, match params and return*/
    } else if (typeIsFunction(Model)) {
        /*fn ptr <=> fn*/
        if (typeIsPtr(DT) && typeIsFunction(DT->base))
            return typeIsCompatible(DT->base, Model);

        else if (!typeIsFunction(DT) || Model->params != DT->params)
            return false;

        for (int i = 0; i < Model->params; i++)
            if (!typeIsEqual(DT->paramTypes[i],
                             Model->paramTypes[i]))
                return false;

        return typeIsEqual(DT->returnType,
                           Model->returnType);

    /*If pointer requested, allow pointers and arrays and basic numeric types*/
    } else if (typeIsPtr(Model)) {
        /*Except for fn pointers*/
        if (typeIsFunction(Model->base))
            return typeIsFunction(DT) && typeIsCompatible(DT, Model->base);

        else
            return    typeIsPtr(DT) || typeIsArray(DT)
                   || (typeIsBasic(DT) && (DT->basic->typeMask & typeNumeric));

    /*If array requested, accept only arrays of matching size and type*/
    } else if (typeIsArray(Model)) {
        return    typeIsArray(DT)
               && (DT->array == Model->array || Model->array < 0 || DT->array < 0)
               && typeIsCompatible(DT->base, Model->base);

    /*Basic type*/
    } else {
        if (typeIsPtr(DT))
            return Model->basic->typeMask & typeNumeric;

        else if (Model->basic->typeMask == typeIntegral)
            return DT->tag == typeBasic && DT->basic->typeMask == typeIntegral;

        else
            return DT->tag == typeBasic && DT->basic == Model->basic;
    }
}

static bool typeQualIsEqual (typeQualifiers L, typeQualifiers R) {
    return L.isConst == R.isConst;
}

bool typeIsEqual (const type* L, const type* R) {
    typeQualifiers Lqual = typeQualifiersCreate(),
                   Rqual = typeQualifiersCreate();
    L = typeTryThroughTypedefQual(L, &Lqual);
    R = typeTryThroughTypedefQual(R, &Rqual);

    if (!typeQualIsEqual(Lqual, Rqual))
        return false;

    else if (typeIsInvalid(L) || typeIsInvalid(R))
        return true;

    else if (L->tag != R->tag)
        return false;

    else if (typeIsFunction(L))
        return typeIsCompatible(L, R);

    else if (typeIsPtr(L))
        return typeIsEqual(L->base, R->base);

    else if (typeIsArray(L))
        return    (L->array == R->array || L->array < 0 || R->array < 0)
               && typeIsEqual(L->base, R->base);

    else {
        assert(typeIsBasic(L));
        return L->basic == R->basic;
    }
}

/*==== Misc. interfaces ====*/

const char* typeTagGetStr (typeTag tag) {
    if (tag == typeBasic) return "typeBasic";
    else if (tag == typePtr) return "typePtr";
    else if (tag == typeArray) return "typeArray";
    else if (tag == typeFunction) return "typeFunction";
    else if (tag == typeInvalid) return "typeInvalid";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("typeTagGetStr", "type tag", str);
        free(str);
        return "<unhandled>";
    }
}

int typeGetSize (const architecture* arch, const type* DT) {
    DT = typeTryThroughTypedef(DT);

    if (typeIsInvalid(DT))
        return 0;

    else if (typeIsArray(DT))
        if (DT->array < 0)
            return 0;

        else
            return DT->array * typeGetSize(arch, DT->base);

    else if (typeIsPtr(DT) || typeIsFunction(DT))
        return arch->wordsize;

    else {
        assert(typeIsBasic(DT));
        return DT->basic->size;
    }
}

char* typeToStr (const type* DT) {
    return typeToStrEmbed(DT, "");
}

char* typeToStrEmbed (const type* DT, const char* embedded) {
    /*TODO: Jump through typedefs and offer akas
            Three modes: print as normal, print jumping through typedefs,
                         print both forms with akas at the correct positions
            Even when directed to, it is to the discretion of this function
            as to whether it is sensible to reprint types*/

    /*Basic type or invalid*/
    if (DT->tag == typeInvalid || DT->tag == typeBasic) {
        char* basicStr = typeIsInvalid(DT)
                            ? "<invalid>"
                            : (DT->basic->ident && DT->basic->ident[0])
                                ? DT->basic->ident
                                : DT->basic->tag == symStruct ? "<unnamed struct>" :
                                  DT->basic->tag == symUnion ? "<unnamed union>" :
                                  DT->basic->tag == symEnum ? "<unnamed enum>" : "<unnamed type>";

        char* qualified = typeQualifiersToStr(DT->qual, basicStr);

        if (embedded[0] == (char) 0)
            return qualified;

        else {
            char* ret = malloc(  strlen(embedded)
                   + strlen(basicStr)
                   + 2 + (DT->qual.isConst ? 6 : 0));
            sprintf(ret, "%s %s", qualified, embedded);
            free(qualified);
            return ret;
        }

    /*Function*/
    } else if (DT->tag == typeFunction) {
        /*Get the param string that goes inside the parens*/

        char* params = 0;

        if (DT->params == 0)
            params = strdup("void");

        else {
            vector/*<char*>*/ paramStrs;
            vectorInit(&paramStrs, DT->params+1);
            vectorPushFromArray(&paramStrs, (void**) DT->paramTypes, DT->params, sizeof(type*));
            vectorMap(&paramStrs, (vectorMapper) typeToStr, &paramStrs);

            if (DT->variadic)
                vectorPush(&paramStrs, "...");

            params = strjoinwith((char**) paramStrs.buffer, paramStrs.length, ", ", malloc);

            if (DT->variadic)
                paramStrs.length--;

            vectorFreeObjs(&paramStrs, free);
        }

        /* */

        char* format = malloc(  strlen(embedded) + 2
                              + strlen(params) + 3);

        if (!embedded[0])
            sprintf(format, "()(%s)", params);

        else
            sprintf(format, "%s(%s)", embedded, params);

        free(params);
        char* ret = typeToStrEmbed(DT->returnType, format);
        free(format);
        return ret;

    /*Array or Ptr*/
    } else {
        char* format;

        if (typeIsPtr(DT)) {
            format = malloc(strlen(embedded) + 4 + (DT->qual.isConst ? 7 : 0));

            char* qualified = typeQualifiersToStr(DT->qual, embedded);

            if (DT->base->tag == typeFunction)
                sprintf(format, "(*%s)", qualified);

            else
                sprintf(format, "*%s", qualified);

            free(qualified);

        } else {
            assert(typeIsArray(DT));

            format = malloc(  strlen(embedded)
                            + (DT->array < 0 ? 0 : logi(DT->array, 10)) + 4);

            if (DT->array < 0)
                sprintf(format, "%s[]", embedded);

            else
                sprintf(format, "%s[%d]", embedded, DT->array);
        }

        char* ret = typeToStrEmbed(DT->base, format);
        free(format);
        return ret;
    }
}

static char* typeQualifiersToStr (typeQualifiers qual, const char* embedded) {
    if (qual.isConst) {
        char* ret = malloc(  strlen("const ")
                           + strlen(embedded) + 1);

        if (embedded[0])
            sprintf(ret, "const %s", embedded);

        else
            strcpy(ret, "const");

        return ret;

    } else
        return strdup(embedded);
}
