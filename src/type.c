#include "../inc/type.h"

#include "../inc/debug.h"
#include "../inc/sym.h"

#include "../inc/architecture.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

static typeQualifiers typeQualifiersCreate ();
static type* typeCreate (typeTag tag);

/**
 * Jump through (immediate) typedefs, possibly recursively, to actual type
 */
static const type* typeTryThroughTypedef (const type* DT);

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
static const type* typeTryThroughTypedefQual (const type* DT, typeQualifiers* qualOut);

static char* typeQualifiersToStr (typeQualifiers qual, const char* embedded);

/*:::: TYPE CTORS/DTOR ::::*/

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
    debugAssert("typeDestroy", "null parameter", DT != 0);

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
    debugAssert("typeDeepDuplicate", "null parameter", DT != 0);

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

/*:::: TYPE MISC HELPERS ::::*/

static const type* typeTryThroughTypedef (const type* DT) {
    if (DT->tag == typeBasic && DT->basic && DT->basic->tag == symTypedef)
        return typeTryThroughTypedef(DT->basic->dt);

    else
        return DT;
}

static const type* typeTryThroughTypedefQual (const type* DT, typeQualifiers* qualOut) {
    if (qualOut)
        qualOut->isConst |= DT->qual.isConst;

    if (DT->tag == typeBasic && DT->basic && DT->basic->tag == symTypedef)
        return typeTryThroughTypedefQual(DT->basic->dt, qualOut);

    else
        return DT;
}

const sym* typeGetRecordSym (const type* record) {
    record = typeTryThroughTypedef(record);

    if (typeIsInvalid(record))
        return 0;

    else {
        debugAssert("typeGetRecordSym", "record param",
                       typeIsRecord(record)
                    || (record->tag == typePtr && typeIsRecord(record->base)));

        if (typeIsPtr(record))
            return record->base->basic;

        else
            return record->basic;
    }
}

/*:::: TYPE DERIVATION ::::*/

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
        debugAssert("typeDeriveFromTwo", "type compatibility", typeIsCompatible(L, R));
        return typeDeriveFrom(L);
    }
}

type* typeDeriveUnified (const type* L, const type* R) {
    if (typeIsInvalid(L))
        return typeDeepDuplicate(R);

    else if (typeIsInvalid(R))
        return typeDeepDuplicate(L);

    else {
        debugAssert("typeDeriveUnified", "type compatibility", typeIsCompatible(L, R));

        if (typeIsEqual(L, R))
            return typeDeepDuplicate(L); //== R

        else
            return typeDeriveFromTwo(L, R);
    }
}

type* typeDeriveBase (const type* DT) {
    DT = typeTryThroughTypedef(DT);

    if (typeIsInvalid(DT))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveBase", "base", typeIsPtr(DT) || typeIsArray(DT));
        return typeDeepDuplicate(DT->base);
    }
}

type* typeDerivePtr (const type* base) {
    return typeCreatePtr(typeDeepDuplicate(base));
}

type* typeDeriveArray (const type* base, int size) {
    return typeCreateArray(typeDeepDuplicate(base), size);
}

type* typeDeriveReturn (const type* fn) {
    fn = typeTryThroughTypedef(fn);

    if (typeIsInvalid(fn))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveReturn", "callable param", typeIsCallable(fn));

        if (typeIsPtr(fn))
            return typeDeriveReturn(fn->base);

        else
            return typeDeepDuplicate(fn->returnType);
    }
}

/*:::: TYPE CLASSIFICATION ::::*/

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
    DT = typeTryThroughTypedef(DT);
    return DT->tag == typeInvalid;
}

bool typeIsVoid (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    /*Is it a built in type of size zero (void)*/
    return    (   (DT->tag == typeBasic && DT->basic->tag == symType)
               && DT->basic->size == 0)
           || typeIsInvalid(DT);
}

bool typeIsRecord (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (   DT->tag == typeBasic
               && (   DT->basic->tag == symStruct
                   || DT->basic->tag == symUnion))
           || typeIsInvalid(DT);
}

bool typeIsStruct (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (   DT->tag == typeBasic && DT->basic->tag == symStruct)
           || typeIsInvalid(DT);
}

bool typeIsCallable (const type* DT) {
    DT = typeTryThroughTypedef(DT);
    return    (   typeIsFunction(DT)
               || (DT->tag == typePtr && typeIsFunction(DT->base)))
           || typeIsInvalid(DT);
}

bool typeIsAssignable (const type* DT) {
    typeQualifiers qual = typeQualifiersCreate();
    DT = typeTryThroughTypedefQual(DT, &qual);
    return typeIsAssignment(DT) && !qual.isConst;
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
    return (DT->tag == typeBasic && (DT->basic->typeMask & typeCondition)) ||
            typeIsPtr(DT) || typeIsInvalid(DT);
}

/*:::: TYPE COMPARISON ::::*/

bool typeIsCompatible (const type* DT, const type* Model) {
    DT = typeTryThroughTypedef(DT);
    Model = typeTryThroughTypedef(Model);

    if (typeIsInvalid(DT) || typeIsInvalid(Model))
        return true;

    /*If function requested, match params and return*/
    else if (typeIsFunction(Model)) {
        /*fn ptr <=> fn*/
        if (typeIsPtr(DT) && typeIsFunction(DT->base))
            typeIsCompatible(DT->base, Model);

        else if (!typeIsFunction(DT) || Model->params != DT->params)
            return false;

        for (int i = 0; i < Model->params; i++)
            if (!typeIsEqual(DT->paramTypes[i],
                             Model->paramTypes[i]))
                return false;

        return typeIsEqual(DT->returnType,
                           Model->returnType);

    /*If pointer requested, allow pointers and arrays and basic numeric types*/
    } else if (typeIsPtr(Model))
        /*Except for fn pointers*/
        if (typeIsFunction(Model->base) && typeIsFunction(DT))
            return typeIsCompatible(DT, Model->base);

        else
            return    typeIsPtr(DT) || typeIsArray(DT)
                   || (typeIsBasic(DT) && (DT->basic->typeMask & typeNumeric));

    /*If array requested, accept only arrays of matching size and type*/
    else if (typeIsArray(Model))
        return    typeIsArray(DT)
               && (DT->array == Model->array || Model->array < 0 || DT->array < 0)
               && typeIsCompatible(DT->base, Model->base);

    /*Basic type*/
    else {
        if (typeIsPtr(DT))
            return Model->basic->typeMask & typeNumeric;

        else
            return !typeIsArray(DT) && DT->basic == Model->basic;
    }
}

bool typeIsEqual (const type* L, const type* R) {
    L = typeTryThroughTypedef(L);
    R = typeTryThroughTypedef(R);

    if (typeIsInvalid(L) || typeIsInvalid(R))
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

    else /*(typeIsBasic(L))*/
        return L->basic == R->basic;
}

/*:::: MISC INTERFACES ::::*/

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
    debugAssert("typeGetSize", "null parameter", DT != 0);

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

    else /*if (typeIsBasic(DT))*/
        return DT->basic->size;
}

char* typeToStr (const type* DT, const char* embedded) {
    /*TODO: Jump through typedefs and offer akas
            Three modes: print as normal, print jumping through typedefs,
                         print both forms with akas at the correct positions
            Even when directed to, it is to the discretion of this function
            as to whether it is sensible to reprint types*/

    /*Basic type or invalid*/
    if (DT->tag == typeInvalid || DT->tag == typeBasic) {
        char* basicStr = typeIsInvalid(DT)
                            ? "<invalid>"
                            : DT->basic->ident;

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

        if (DT->params != 0) {
            char** paramStrs = calloc(DT->params, sizeof(char*));
            int length = 1;

            /*Get strings for all the params and count total string length*/
            for (int i = 0; i < DT->params; i++) {
                paramStrs[i] = typeToStr(DT->paramTypes[i], "");
                length += strlen(paramStrs[i])+2;
            }

            /*Cat them together*/

            params = malloc(length+1);

            int charno = 0;

            for (int i = 0; i < DT->params-1; i++) {
                charno += sprintf(params+charno, "%s, ", paramStrs[i]);
                free(paramStrs[i]);
            }

            /*Cat the final one, sans the delimiting comma*/
            sprintf(params+charno, "%s", paramStrs[DT->params-1]);
            free(paramStrs[DT->params-1]);
            free(paramStrs);

        } else
            params = strdup("void");

        /* */

        char* format = malloc(strlen(embedded) +
                              strlen(params)+5);

        if (embedded[0] == 0)
            sprintf(format, "()(%s)", params);

        else
            sprintf(format, "%s(%s)", embedded, params);

        free(params);
        char* ret = typeToStr(DT->returnType, format);
        free(format);
        return ret;

    /*Array or Ptr*/
    } else {
        char* format = 0;

        if (typeIsPtr(DT)) {
            format = malloc(strlen(embedded) + 4 + (DT->qual.isConst ? 7 : 0));

            char* qualified = typeQualifiersToStr(DT->qual, embedded);

            if (DT->base->tag == typeFunction)
                sprintf(format, "(*%s)", qualified);

            else
                sprintf(format, "*%s", qualified);

            free(qualified);

        } else /*if (typeIsArray(DT))*/ {
            format = malloc(  strlen(embedded)
                            + (DT->array < 0 ? 0 : logi(DT->array, 10)) + 4);

            if (DT->array < 0)
                sprintf(format, "%s[]", embedded);

            else
                sprintf(format, "%s[%d]", embedded, DT->array);
        }

        char* ret = typeToStr(DT->base, format);
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
