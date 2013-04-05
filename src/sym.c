#include "../inc/sym.h"

#include "../inc/debug.h"
#include "../inc/type.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

static sym* symCreate (symTag tag, sym* Parent);
static void symDestroy (sym* Symbol);

static void symAddChild (sym* Parent, sym* Child);

sym* symInit () {
    return symCreate(symScope, 0);
}

void symEnd (sym* Global) {
    symDestroy(Global);
}

static sym* symCreate (symTag tag, sym* Parent) {
    sym* Symbol = malloc(sizeof(sym));
    Symbol->tag = tag;
    Symbol->ident = 0;

    Symbol->proto = false;

    Symbol->storage = storageAuto;
    Symbol->dt = 0;

    Symbol->size = 0;

    symAddChild(Parent, Symbol);
    Symbol->firstChild = 0;
    Symbol->lastChild = 0;
    Symbol->nextSibling = 0;

    Symbol->label = operandCreateLabel(0);
    Symbol->offset = 0;

    return Symbol;
}

static void symDestroy (sym* Symbol) {
    free(Symbol->ident);

    if (Symbol->firstChild)
        symDestroy(Symbol->firstChild);

    if (Symbol->nextSibling)
        symDestroy(Symbol->nextSibling);

    if (Symbol->dt)
        typeDestroy(Symbol->dt);

    free(Symbol);
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
    Symbol->ident = strdup(ident);
    Symbol->typeMask = typeAssignment;
    return Symbol;
}

sym* symCreateId (sym* Parent, char* ident) {
    sym* Symbol = symCreate(symId, Parent);
    Symbol->ident = strdup(ident);
    Symbol->proto = true;
    return Symbol;
}

sym* symCreateParam (sym* Parent, char* ident) {
    sym* Symbol = symCreate(symParam, Parent);
    Symbol->ident = strdup(ident);
    return Symbol;
}

static void symAddChild (sym* Parent, sym* Child) {
    /*Global namespace?*/
    if (!Parent && Child->tag == symScope) {
        Child->parent = 0;
        return;

    } else if (!Child || !Parent) {
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
}

sym* symChild (const sym* Scope, const char* Look) {
    //printf("searching: %s\n", Scope->ident);

    for (sym* Current = Scope->firstChild;
            Current;
            Current = Current->nextSibling) {
        //reportSymbol(Current);
        //getchar();

        if (Current->ident && !strcmp(Current->ident, Look))
            return Current;

        /*Children of enums are visible from their parents scope*/
        if (Current->tag == symEnum) {
            sym* Found = symChild(Current, Look);

            if (Found)
                return Found;
        }
    }

    return 0;
}

sym* symFind (const sym* Scope, const char* Look) {
    //printf("look: %s\n", Look);

    /*Search the current namespace and all its ancestors*/
    for (;
         Scope;
         Scope = Scope->parent) {
        sym* Found = symChild(Scope, Look);

        if (Found) {
            //printf("found: %s\n", Found->ident);
            return Found;

        } /*else
            printf("not found in %s\n", Scope->ident);*/
    }

    //puts("find failed");

    return 0;
}

const char* symTagGetStr (symTag tag) {
    if (tag == symUndefined)
        return "symUndefined";
    else if (tag == symScope)
        return "symScope";
    else if (tag == symType)
        return "symType";
    else if (tag == symStruct)
        return "symStruct";
    else if (tag == symEnum)
        return "symEnum";
    else if (tag == symId)
        return "symId";
    else if (tag == symParam)
        return "symParam";

    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("symTagGetStr", "symbol tag", str);
        free(str);
        return "unhandled";
    }
}

const char* storageTagGetStr (storageTag tag) {
    if (tag == storageUndefined)
        return "storageUndefined";
    else if (tag == storageAuto)
        return "storageAuto";
    else if (tag == storageRegister)
        return "storageRegister";
    else if (tag == storageStatic)
        return "storageStatic";
    else if (tag == storageExtern)
        return "storageExtern";

    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("storageTagGetStr", "symbol tag", str);
        free(str);
        return "undefined";
    }
}
