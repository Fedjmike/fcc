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

    vectorInit(&Symbol->decls, 2);
    Symbol->impl = 0;

    Symbol->storage = storageAuto;
    Symbol->dt = 0;

    Symbol->size = 0;
    Symbol->typeMask = typeNone;
    Symbol->complete = false;

    symAddChild(Parent, Symbol);
    Symbol->firstChild = 0;
    Symbol->lastChild = 0;
    Symbol->nextSibling = 0;
    Symbol->children = 0;

    Symbol->label = 0;
    Symbol->offset = 0;

    return Symbol;
}

static void symDestroy (sym* Symbol) {
    free(Symbol->ident);

    vectorFree(&Symbol->decls);

    if (Symbol->firstChild)
        symDestroy(Symbol->firstChild);

    if (Symbol->nextSibling)
        symDestroy(Symbol->nextSibling);

    if (Symbol->dt)
        typeDestroy(Symbol->dt);

    free(Symbol->label);
    free(Symbol);
}

sym* symCreateScope (sym* Parent) {
    return symCreate(symScope, Parent);
}

sym* symCreateType (sym* Parent, const char* ident, int size, symTypeMask typeMask) {
    sym* Symbol = symCreate(symType, Parent);
    Symbol->ident = strdup(ident);
    Symbol->size = size;
    Symbol->typeMask = typeMask;
    Symbol->complete = true;
    return Symbol;
}

sym* symCreateNamed (symTag tag, sym* Parent, const char* ident) {
    sym* Symbol = symCreate(tag, Parent);
    Symbol->ident = strdup(ident);

    if (tag == symStruct)
        Symbol->typeMask = typeStruct;

    else if (tag == symEnum)
        Symbol->typeMask = typeEnum;

    return Symbol;
}

static void symAddChild (sym* Parent, sym* Child) {
    /*Global namespace?*/
    if (!Parent && Child->tag == symScope) {
        Child->parent = 0;
        return;

    } else if (!Child || !Parent) {
        printf("symAddChild(): null %s given.\n",
               Child ? "parent" : "child");
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
    Parent->children++;
}

const sym* symGetChild (const sym* Parent, int n) {
    sym* Current = Parent->firstChild;

    for (int i = 0;
         i < n && Current;
         i++, Current = Current->nextSibling)
        {}

    return Current;
}

sym* symChild (const sym* Scope, const char* look) {
    debugAssert("symChild", "null scope", Scope != 0);
    debugAssert("symChild", "null string", look != 0);

    //printf("searching: %s\n", Scope->ident);

    for (sym* Current = Scope->firstChild;
            Current;
            Current = Current->nextSibling) {
        //reportSymbol(Current);
        //getchar();

        if (Current->ident && !strcmp(Current->ident, look))
            return Current;

        /*Anonymous inside a struct/union?*/
        if (   Current->ident && Current->ident[0] == 0
            && (   Current->parent->tag == symStruct
                || Current->parent->tag == symUnion)) {
            sym* Found = symChild(Current, look);

            if (Found)
                return Found;
        }
    }

    return 0;
}

sym* symFind (const sym* Scope, const char* look) {
    debugAssert("symFind", "null scope", Scope != 0);
    debugAssert("symFind", "null string", look != 0);

    //printf("look: %s\n", look);

    /*Search the current namespace and all its ancestors*/
    for (;
         Scope;
         Scope = Scope->parent) {
        sym* Found = symChild(Scope, look);

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
    if (tag == symUndefined) return "undefined";
    else if (tag == symScope) return "scope";
    else if (tag == symType) return "type";
    else if (tag == symTypedef) return "typedef";
    else if (tag == symStruct) return "struct";
    else if (tag == symUnion) return "union";
    else if (tag == symEnum) return "enum";
    else if (tag == symEnumConstant) return "enum constant";
    else if (tag == symId) return "id";
    else if (tag == symParam) return "parameter";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("symTagGetStr", "symbol tag", str);
        free(str);
        return "<unhandled>";
    }
}

const char* storageTagGetStr (storageTag tag) {
    if (tag == storageUndefined) return "storageUndefined";
    else if (tag == storageAuto) return "storageAuto";
    else if (tag == storageStatic) return "storageStatic";
    else if (tag == storageExtern) return "storageExtern";
    else if (tag == storageTypedef) return "storageTypedef";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("storageTagGetStr", "storage tag", str);
        free(str);
        return "<undefined>";
    }
}
