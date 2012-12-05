#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "stdio.h"

#include "../inc/type.h"
#include "../inc/sym.h"

type typeCreate (sym* basic, int ptr, int array) {
    type DT = {basic, ptr, array};
    return DT;
}

int typeGetSize (type DT) {
    if (DT.ptr == 0)
        return DT.basic->size *
               (DT.array == 0 ? 1 : DT.array);

    else
        return symFindGlobal("void ptr")->size;
}

char* typeToStr (type DT) {
    char* Str = malloc(strlen(DT.basic->ident) + DT.ptr + log10(DT.array) + 10);

    sprintf(Str, "%s", DT.basic ? DT.basic->ident : 0);

    for (int i = 0; i < DT.ptr; i++)
        strcat(Str, "*");

    if (DT.array != 0)
        sprintf(Str, "[%d]", DT.array);

    return Str;
}

bool typeIsPtr (type DT) {
    return DT.ptr >= 1 && DT.array == 0;
}

type typeDerefPtr (type DT) {
    return typeCreate(DT.basic, DT.ptr-1, 0);
}

bool typeIsRecord (type DT) {
    return DT.basic->class != symStruct &&
            DT.array == 0;
}
