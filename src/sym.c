#include "../inc/debug.h"
#include "../inc/sym.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

static void symAddChild (sym* Parent, sym* Child);
static void symDestroy (sym* Symbol);

sym* symInit () {
    return symCreate(symScope, 0);
}

void symEnd (sym* Global) {
    symDestroy(Global);
}

sym* symCreate (symClass class, sym* Parent) {
    sym* Symbol = malloc(sizeof(sym));
    Symbol->class = class;
    Symbol->ident = 0;

    Symbol->proto = false;

    Symbol->storage = storageAuto;
    Symbol->dt.basic = 0;
    Symbol->dt.ptr = 0;
    Symbol->dt.array = 0;

    Symbol->size = 0;

    symAddChild(Parent, Symbol);
    Symbol->firstChild = 0;
    Symbol->lastChild = 0;
    Symbol->nextSibling = 0;

    Symbol->params = 0;

    Symbol->label = operandCreateLabel(0);
    Symbol->offset = 0;

    return Symbol;
}

sym* symCreateType (sym* Parent, char* ident, int size, symTypeMask typeMask) {
    sym* Symbol = symCreate(symType, Parent);
    Symbol->ident = strdup(ident);
    Symbol->size = size;
    Symbol->typeMask = typeMask;
    return Symbol;
}

sym* symCreateStruct (sym* Parent, char* ident) {
    sym* Symbol = symCreate(symStruct, Parent);
    Symbol->ident = ident;
    Symbol->typeMask = typeAssignment;
    return Symbol;
}

sym* symCreateVar (sym* Parent, char* ident, type DT, storageClass storage) {
    sym* Symbol = symCreate(symVar, Parent);
    Symbol->ident = ident;
    Symbol->dt = DT;
    Symbol->storage = storage;
    return Symbol;
}

static void symAddChild (sym* Parent, sym* Child) {
    /*Global namespace?*/
    if (!Parent && Child->class == symScope)
        return;

    else if (!Child || !Parent) {
        printf("symAddChild(): null %s given.\n",
               !Parent ? "parent" : "child");
        return;
    }

    if (Parent->firstChild == 0) {
        Parent->firstChild = Child;
        Parent->lastChild = Child;

    } else {
        Parent->lastChild->nextSibling = Child;
        Parent->lastChild = Child;
    }

    Child->parent = Parent;

    if (Child->class == symParam)
        Parent->params++;
}

static void symDestroy (sym* Symbol) {
    free(Symbol->ident);

    if (Symbol->firstChild)
        symDestroy(Symbol->firstChild);

    if (Symbol->nextSibling)
        symDestroy(Symbol->nextSibling);

    free(Symbol);
}

sym* symChild (sym* Scope, char* Look) {
    //printf("searching: %s\n", Scope->ident);

    for (sym* Current = Scope->firstChild;
            Current;
            Current = Current->nextSibling) {
        //reportSymbol(Current);
        //getchar();

        if (Current->ident && !strcmp(Current->ident, Look))
            return Current;

        /*Children of enums are visible from their parents scope*/
        if (Current->class == symEnum) {
            sym* Found = symChild(Current, Look);

            if (Found)
                return Found;
        }
    }

    return 0;
}

sym* symFind (sym* Scope, char* Look) {
    //printf("look: %s\n", Look);

    /*Search the current namespace and all its ancestors*/
    for (;
         Scope;
         Scope = Scope->parent) {
        sym* Found = symChild(Scope, Look);

        if (Found) {
            //printf("found: %s\n", Found->ident);
            return Found;
        }
    }

    return 0;
}

const char* symClassGetStr (symClass class) {
    if (class == symUndefined)
        return "symUndefined";
    else if (class == symScope)
        return "symScope";
    else if (class == symType)
        return "symType";
    else if (class == symStruct)
        return "symStruct";
    else if (class == symEnum)
        return "symEnum";
    else if (class == symFunction)
        return "symFunction";
    else if (class == symParam)
        return "symParam";
    else if (class == symVar)
        return "symVar";

    else {
        char* Str = malloc(class+1);
        sprintf(Str, "%d", class);
        debugErrorUnhandled("symClassGetStr", "symbol class", Str);
        free(Str);
        return "unhandled";
    }
}

const char* storageClassGetStr (storageClass class) {
    if (class == storageUndefined)
        return "storageUndefined";

    else if (class == storageAuto)
        return "storageAuto";

    else if (class == storageRegister)
        return "storageRegister";

    else if (class == storageStatic)
        return "storageStatic";

    else if (class == storageExtern)
        return "storageExtern";

    else {
        char* Str = malloc(class+1);
        sprintf(Str, "%d", class);
        debugErrorUnhandled("storageClassGetStr", "storage class", Str);
        free(Str);
        return "undefined";
    }
}
