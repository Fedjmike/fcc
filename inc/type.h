#pragma once

#include "../std/std.h"

struct sym;

typedef struct {
    struct sym* basic;
    int ptr;    /*Levels of indirection. e.g. char*** would be 3.*/
    int array;  /*Array size, 0 if not array, -1 if unspecified*/
    bool lvalue;
} type;

type typeCreate (struct sym* basic, int ptr, int array, bool lvalue);
type typeCreateFromBasic (struct sym* basic);
type typeCreateFrom (type DT);
type typeCreateFromTwo (type L, type R);
type typeCreateUnified (type L, type R);
type typeCreateLValueFromPtr (type DT);
type typeCreatePtrFromLValue (type DT);
type typeCreateElementFromArray (type DT);
type typeCreateArrayFromElement (type DT, int array);

bool typeIsVoid (type DT);

bool typeIsPtr (type DT);
bool typeIsArray (type DT);
bool typeIsRecord (type DT);
bool typeIsLValue (type DT);

bool typeIsNumeric (type DT);
bool typeIsOrdinal (type DT);
bool typeIsEquality (type DT);
bool typeIsAssignment (type DT);

bool typeIsCompatible (type DT, type Model);
bool typeIsEqual (type L, type R);

int typeGetSize (type DT);
char* typeToStr (type DT);
