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

    vectorInit(&Symbol->children, 4);
    symAddChild(Parent, Symbol);

    Symbol->label = 0;
    Symbol->offset = 0;

    return Symbol;
}

static void symDestroy (sym* Symbol) {
    free(Symbol->ident);

    vectorFree(&Symbol->decls);

    vectorFreeObjs(&Symbol->children, (vectorDtor) symDestroy);

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

    else if (tag == symUnion)
        Symbol->typeMask = typeUnion;

    else if (tag == symEnum)
        Symbol->typeMask = typeEnum;

    return Symbol;
}

static void symAddChild (sym* Parent, sym* Child) {
    if (debugAssert("symAddChild", "null child", Child != 0))
        ;

    /*Global namespace?*/
    else if (!Parent && Child->tag == symScope)
        Child->parent = 0;

    else if (debugAssert("symAddChild", "null parent", Parent != 0))
        ;

    else {
        vectorPush(&Parent->children, Child);
        Child->parent = Parent;
        Child->nthChild = Parent->children.length-1;
    }
}

sym* symChild (const sym* Scope, const char* look) {
    if (   debugAssert("symChild", "null scope", Scope != 0)
        || debugAssert("symChild", "null string", look != 0))
        return 0;

    //printf("searching: %s\n", Scope->ident);

    for (int n = 0; n < Scope->children.length; n++) {
        sym* Current = vectorGet(&Scope->children, n);
        //reportSymbol(Current);
        //getchar();

        if (Current->ident && !strcmp(Current->ident, look))
            return Current;

        /*Anonymous inside a struct/union?*/
        if (   Current->ident && !Current->ident[0]
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
    if (   debugAssert("symFind", "null scope", Scope != 0)
        || debugAssert("symFind", "null string", look != 0))
        return 0;

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
