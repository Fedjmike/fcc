#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"

/*:::: TYPE CONSTRUCTION ::::*/

type typeCreate (sym* basic, int ptr, int array, bool lvalue) {
    type DT = {basic, ptr, array, lvalue};
    return DT;
}

type typeCreateFromBasic (struct sym* basic) {
    return typeCreate(basic, 0, 0, false);
}

type typeCreateFrom (type DT) {
    return typeCreate(DT.basic, DT.ptr, DT.array, false);
}

type typeCreateFromTwo (type L, type R) {
    debugAssert("typeCreateFromTwo", "type compatibility", typeIsCompatible(L, R));
    return typeCreate(L.basic, L.ptr, L.array, false);
}

type typeCreateUnified (type L, type R) {
    debugAssert("typeCreateUnified", "type compatibility", typeIsCompatible(L, R));

    if (typeIsEqual(L, R))
        return L; //== R

    else {
        /*Currently (?) the only way for compatible types not to be
          equal is for one to be an lvalue and one an rvalue.
          Lower to common denominator!*/
        type DT = L;
        DT.lvalue = false;
        return DT;
    }
}

type typeCreateLValueFromPtr (type DT) {
    debugAssert("typeCreateLValueFromPtr", "pointer", typeIsPtr(DT));

    DT.ptr--;
    DT.lvalue = true;
    return DT;
}

type typeCreatePtrFromLValue (type DT) {
    debugAssert("typeCreatePtrFromLValue", "lvalue", typeIsLValue(DT));

    DT.ptr++;
    DT.lvalue = false;
    return DT;
}

type typeCreateElementFromArray (type DT) {
    debugAssert("typeCreateElementFromArray", "array", typeIsArray(DT));

    DT.array = 0;
    return DT;
}

type typeCreateArrayFromElement (type DT, int array) {
    if (typeIsArray(DT))
        debugErrorUnhandled("typeCreateArrayFromElement", "type", "array");

    DT.array = array;
    return DT;
}

/*:::: TYPE CLASSIFICATION ::::*/

bool typeIsVoid (type DT) {
    return typeGetSize(DT) == 0;
}

bool typeIsPtr (type DT) {
    return DT.ptr >= 1 && !typeIsArray(DT);
}

bool typeIsArray (type DT) {
    return DT.array >= 1 || DT.array == -1;
}

bool typeIsRecord (type DT) {
    return DT.basic->class == symStruct &&
            !typeIsPtr(DT) && !typeIsArray(DT);
}

bool typeIsLValue (type DT) {
    return DT.lvalue;
}

bool typeIsNumeric (type DT) {
    return ((DT.basic->typeMask && typeNumeric) || !typeIsPtr(DT)) &&
            !typeIsArray(DT);
}

bool typeIsOrdinal (type DT) {
    return ((DT.basic->typeMask && typeOrdinal) || !typeIsPtr(DT)) &&
            !typeIsArray(DT);
}

bool typeIsEquality (type DT) {
    return ((DT.basic->typeMask && typeEquality) || !typeIsPtr(DT)) &&
            !typeIsArray(DT);
}
bool typeIsAssignment (type DT) {
    return ((DT.basic->typeMask && typeAssignment) || !typeIsPtr(DT)) &&
            !typeIsArray(DT);
}

/*:::: TYPE COMPARISON ::::*/

bool typeIsCompatible (type DT, type Model) {
    /*If pointer requested, allow pointers or arrays (of matching type)*/
    if (typeIsPtr(Model)) {
        if (typeIsArray(DT))
            return typeIsCompatible(typeCreateElementFromArray(DT),
                                    typeCreateLValueFromPtr(Model));

        else if (typeIsPtr(DT))
            return typeIsCompatible(typeCreateLValueFromPtr(DT),
                                    typeCreateLValueFromPtr(Model));

        else
            return false;

    /*If array requested, accept only arrays of matching size and type*/
    } else if (typeIsArray(Model))
        return typeIsArray(DT) &&
            ((DT.array == Model.array) || Model.array == -1) &&
            typeIsCompatible(typeCreateElementFromArray(DT),
                             typeCreateElementFromArray(Model));

    /*Basic type*/
    else
        return !typeIsPtr(DT) && !typeIsArray(DT) &&
            (DT.basic == Model.basic);
}

bool typeIsEqual (type L, type R) {
    return L.array == R.array &&
           L.ptr == R.ptr &&
           L.basic == R.basic &&
           L.lvalue == R.lvalue;
}

/*:::: MISC INTERFACES ::::*/

int typeGetSize (type DT) {
    if (DT.ptr == 0)
        return DT.basic->size *
               (DT.array == 0 ? 1 : DT.array);

    else
        return 8; //yummy magic number
}

char* typeToStr (type DT) {
    char* Str = malloc(strlen(DT.basic->ident) + DT.ptr + DT.array + 10);

    sprintf(Str, "%s", DT.basic ? DT.basic->ident : "undefined");

    for (int i = 0; i < DT.ptr; i++)
        strcat(Str, "*");

    if (DT.array != 0) {
        char* ArrStr = malloc(DT.array + 10);
        sprintf(ArrStr, "[%d]", DT.array);
        strcat(Str, ArrStr);
        free(ArrStr);
    }

    if (DT.lvalue)
        strcat(Str, "&");

    return Str;
}
