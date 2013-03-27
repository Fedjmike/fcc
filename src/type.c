#include "../inc/type.h"

#include "../inc/debug.h"
#include "../inc/sym.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

static type* typeCreate ();

/**
 * Returns (int) floor(log((double) x) / log((double) base))
 */
static int logi (int x, int base) {
    int n = 0;

    for (; x >= base; n++)
        x /= base;

    return n;
}

/*:::: TYPE CTORS/DTOR ::::*/

static type* typeCreate (typeClass class) {
    type* DT = malloc(sizeof(type));
    DT->class = class;
    DT->basic = 0;

    DT->base = 0;
    DT->array = 0;

    DT->returnType = 0;
    DT->paramTypes = 0;
    DT->params = 0;

    return DT;
}

type* typeCreateBasic (struct sym* basic) {
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

type* typeCreateFunction (type* returnType, type** paramTypes, int params) {
    type* DT = typeCreate(typeFunction);
    DT->returnType = returnType;
    DT->paramTypes = paramTypes;
    DT->params = params;
    return DT;
}

type* typeCreateInvalid () {
    return typeCreate(typeInvalid);
}

void typeDestroy (type* DT) {
    debugAssert("typeDestroy", "null parameter", DT != 0);

    if (typeIsBasic(DT) || typeIsInvalid(DT))
        ;

    else if (typeIsPtr(DT) || typeIsArray(DT))
        typeDestroy(DT->base);

    else if (typeIsFunction(DT)) {
        typeDestroy(DT->returnType);

        for (int i = 0; i < DT->params; i++)
            typeDestroy(DT->paramTypes[i]);

        free(DT->paramTypes);
    }

    free(DT);
}

type* typeDeepDuplicate (const type* Old) {
    debugAssert("typeDeepDuplicate", "null parameter", Old != 0);

    if (typeIsInvalid(Old))
        return typeCreateInvalid();

    else {
        /*Plain copy of type*/

        type* New = malloc(sizeof(type));
        memcpy(New, Old, sizeof(type));

        /*Recursively duplicate subtypes*/

        if (typeIsPtr(New) || typeIsArray(New))
            New->base = typeDeepDuplicate(New->base);

        else if (typeIsFunction(New)) {
            New->returnType = typeDeepDuplicate(New->returnType);

            for (int i = 0; i < New->params; i++)
                New->paramTypes[i] = typeDeepDuplicate(New->paramTypes[i]);
        }

        return New;
    }
}

/*:::: TYPE DERIVATION ::::*/

type* typeDeriveFrom (const type* DT) {
    type* New = typeDeepDuplicate(DT);
    return New;
}

type* typeDeriveFromTwo (const type* L, const type* R) {
    if (typeIsInvalid(L) || typeIsInvalid(R))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveFromTwo", "type compatibility", typeIsCompatible(L, R));
        return typeDeriveFrom(L);
    }
}

type* typeDeriveUnified (const type* L, const type* R) {
    if (typeIsInvalid(L) || typeIsInvalid(R))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveUnified", "type compatibility", typeIsCompatible(L, R));

        if (typeIsEqual(L, R))
            return typeDeepDuplicate(L); //== R

        else
            return typeDeriveFromTwo(L, R);
    }
}

type* typeDeriveBase (const type* DT) {
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

type* typeDeriveReturn (const type* DT) {
    if (typeIsInvalid(DT))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveReturn", "function", typeIsFunction(DT));
        return typeDeepDuplicate(DT->returnType);
    }
}

/*:::: TYPE CLASSIFICATION ::::*/

bool typeIsBasic (const type* DT) {
    return DT->class == typeBasic || typeIsInvalid(DT);
}

bool typeIsPtr (const type* DT) {
    return DT->class == typePtr || typeIsInvalid(DT);
}

bool typeIsArray (const type* DT) {
    return DT->class == typeArray || typeIsInvalid(DT);
}

bool typeIsFunction (const type* DT) {
    return DT->class == typeFunction || typeIsInvalid(DT);
}

bool typeIsInvalid (const type* DT) {
    return DT->class == typeInvalid;
}

bool typeIsVoid (const type* DT) {
    /*Is it a built in type of size zero (void)*/
    return    (   (DT->class == typeBasic && DT->basic->class == symType)
               && typeGetSize(DT) == 0)
           || typeIsInvalid(DT);
}

bool typeIsRecord (const type* DT) {
    return    (DT->class == typeBasic && DT->basic->class == symStruct)
           || typeIsInvalid(DT);
}

bool typeIsCallable (const type* DT) {
    return    (   typeIsFunction(DT)
               || (DT->class == typePtr && typeIsFunction(DT->base)))
           || typeIsInvalid(DT);
}

bool typeIsNumeric (const type* DT) {
    return    (DT->class == typeBasic && (DT->basic->typeMask & typeNumeric))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsOrdinal (const type* DT) {
    return    (DT->class == typeBasic && (DT->basic->typeMask & typeOrdinal))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsEquality (const type* DT) {
    return    (DT->class == typeBasic && (DT->basic->typeMask & typeEquality))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsAssignment (const type* DT) {
    return    (DT->class == typeBasic && (DT->basic->typeMask & typeAssignment))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsCondition (const type* DT) {
    return (DT->class == typeBasic && (DT->basic->typeMask & typeCondition)) ||
            typeIsPtr(DT) || typeIsInvalid(DT);
}

/*:::: TYPE COMPARISON ::::*/

bool typeIsCompatible (const type* DT, const type* Model) {
    if (typeIsInvalid(DT) || typeIsInvalid(Model))
        return true;

    /*If function requested, match params and return*/
    else if (typeIsFunction(Model)) {
        if (Model->params != DT->params)
            return false;

        for (int i = 0; i < Model->params; i++)
            if (!typeIsEqual(DT->paramTypes[i],
                             Model->paramTypes[i]))
                return false;

        return typeIsEqual(DT->returnType,
                           Model->returnType);

    /*If pointer requested, allow pointers or arrays (of matching type)*/
    } else if (typeIsPtr(Model))
        return    (typeIsPtr(DT) || typeIsArray(DT))
               && typeIsCompatible(DT->base, Model->base);

    /*If array requested, accept only arrays of matching size and type*/
    else if (typeIsArray(Model))
        return    typeIsArray(DT)
               && ((DT->array == Model->array) || Model->array == -1)
               && typeIsCompatible(DT->base, Model->base);

    /*Basic type*/
    else
        return    !typeIsPtr(DT) && !typeIsArray(DT)
               && (DT->basic == Model->basic);
}

bool typeIsEqual (const type* L, const type* R) {
    if (typeIsInvalid(L) || typeIsInvalid(R))
        return true;

    else if (L->class != R->class)
        return false;

    else if (typeIsFunction(L))
        return typeIsCompatible(L, R);

    else if (typeIsPtr(L))
        return typeIsEqual(L->base, R->base);

    else if (typeIsArray(L))
        return    L->array == R->array
               && typeIsEqual(L->base, R->base);

    else
        return L->basic == R->basic;
}

/*:::: MISC INTERFACES ::::*/

const char* typeClassGetStr (typeClass class) {
    if (class == typeBasic)
        return "typeBasic";
    else if (class == typePtr)
        return "typePtr";
    else if (class == typeArray)
        return "typeArray";
    else if (class == typeFunction)
        return "typeFunction";
    else if (class == typeInvalid)
        return "typeInvalid";

    else {
        char* Str = malloc(class+1);
        sprintf(Str, "%d", class);
        debugErrorUnhandled("typeClassGetStr", "type class", Str);
        free(Str);
        return "unhandled";
    }
}

int typeGetSize (const type* DT) {
    if (typeIsInvalid(DT))
        return 0;

    else if (typeIsArray(DT))
        return DT->array * typeGetSize(DT->base);

    else if (typeIsPtr(DT) || typeIsFunction(DT))
        return 8; //yummy magic number

    else /*if (typeIsBasic(DT))*/
        return DT->basic->size;
}

char* typeToStr (const type* DT, const char* embedded) {
    if (typeIsInvalid(DT))
        return strdup("<invalid>");

    /*Basic type*/
    else if (typeIsBasic(DT)) {
        if (embedded[0] == 0)
            return strdup(DT->basic->ident);

        else {
            char* ret = malloc(strlen(embedded) +
                               strlen(DT->basic->ident)+2);

            sprintf(ret, "%s %s", DT->basic->ident, embedded);
            return ret;
        }

    /*Function*/
    } else if (typeIsFunction(DT)) {
        /*Get the param string that goes inside the parens*/

        char* params = 0;

        if (DT->params != 0) {
            char* paramStrs[DT->params];
            int length = 1;

            /*Get strings for all the params and count total string length*/
            for (int i = 0; i < DT->params; i++) {
                paramStrs[i] = typeToStr(DT->paramTypes[i], "");
                length += strlen(paramStrs[i])+2;
            }

            /*Cat them together*/

            params = malloc(length+1);

            int charno = 0;

            for (int i = 0; i < DT->params-1; i++)
                charno += sprintf(params+charno, "%s, ", paramStrs[i]);

            /*Cat the final one, sans the delimiting comma*/
            sprintf(params+charno, "%s", paramStrs[DT->params-1]);

        } else
            params = strdup("void");

        /* */

        char* format = malloc(strlen(embedded) +
                              strlen(params)+6);
        sprintf(format, "(*%s)(%s)", embedded, params);
        free(params);
        char* ret = typeToStr(DT->returnType, format);
        free(format);
        return ret;

    /*Array or Ptr*/
    } else {
        char* format = 0;

        if (typeIsPtr(DT)) {
            format = malloc(strlen(embedded)+2);
            sprintf(format, "*%s", embedded);

        } else /*if (typeIsArray(DT))*/ {
            format = malloc(strlen(embedded) +
                            logi(DT->array, 10)+4);

            if (DT->array == -1)
                sprintf(format, "%s[]", embedded);

            else
                sprintf(format, "%s[%d]", embedded, DT->array);
        }

        char* ret = typeToStr(DT->base, format);
        free(format);
        return ret;
    }
}
