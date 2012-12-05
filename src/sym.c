#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "../inc/debug.h"
#include "../inc/sym.h"

static sym* Global = 0;

sym* symInit () {
    Global = symCreate(symGlobal);
    Global->ident = "global namespace";

    return Global;
}

void symEnd () {
    symDestroy(Global);
}

sym* symCreate (symClass class) {
    sym* Symbol = malloc(sizeof(sym));
    Symbol->class = class;
    Symbol->ident = 0;
    Symbol->proto = false;

    Symbol->storage = storageAuto;
    Symbol->dt.basic = 0;
    Symbol->dt.ptr = 0;
    Symbol->dt.array = 0;

    Symbol->size = 0;

    Symbol->parent = 0;
    Symbol->firstChild = 0;
    Symbol->lastChild = 0;
    Symbol->nextSibling = 0;

    Symbol->label = operandCreateLabel(0);
    Symbol->offset = 0;

    return Symbol;
}

sym* symCreateDataType (char* Ident, int Size) {
    sym* Symbol = symCreate(symType);
    Symbol->ident = strdup(Ident);
    Symbol->size = Size;

    symAddChild(Global, Symbol);

    return Symbol;
}

void symDestroy (sym* Symbol) {
    free(Symbol->ident);

    if (Symbol->firstChild)
        symDestroy(Symbol->firstChild);

    if (Symbol->nextSibling)
        symDestroy(Symbol->nextSibling);

    free(Symbol);
}

void symAddChild (sym* Parent, sym* Child) {
    if (Parent->firstChild == 0) {
        Parent->firstChild = Child;
        Parent->lastChild = Child;

    } else {
        Parent->lastChild->nextSibling = Child;
        Parent->lastChild = Child;
    }

    Child->parent = Parent;
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
    }

    return 0;
}

sym* symFind (sym* Scope, char* Look) {
    //printf("look: %s\n", Look);

    for (;
         Scope;
         Scope = Scope->parent) {
        sym* Found = symChild(Scope, Look);

        if (Found) {
            //printf("found: %s\n", Found->ident);
            return Found;
        }
    }

    return symChild(Global, Look);;
}

sym* symFindGlobal (char* Look) {
    return symChild(Global, Look);
}
