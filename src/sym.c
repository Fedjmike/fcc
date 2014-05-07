#include "../inc/sym.h"

#include "../inc/debug.h"
#include "../inc/type.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

static sym* symCreate (symTag tag);
static sym* symCreateParented (symTag tag, sym* Parent);
static void symDestroy (sym* Symbol);

static sym* symCreateLink (sym* Symbol);

static void symAddChild (sym* Parent, sym* Child);

sym* symInit () {
    return symCreate(symScope);
}

void symEnd (sym* Global) {
    symDestroy(Global);
}

static sym* symCreate (symTag tag) {
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
    Symbol->parent = 0;

    Symbol->label = 0;
    Symbol->offset = 0;
    Symbol->constValue = 0;

    return Symbol;
}

static sym* symCreateParented (symTag tag, sym* Parent) {
    sym* Symbol = symCreate(tag);
    symAddChild(Parent, Symbol);
    return Symbol;
}

static void symDestroy (sym* Symbol) {
    free(Symbol->ident);

    vectorFree(&Symbol->decls);

    if (Symbol->tag != symModuleLink && Symbol->tag != symLink)
        vectorFreeObjs(&Symbol->children, (vectorDtor) symDestroy);

    else
        vectorFree(&Symbol->children);

    if (Symbol->dt)
        typeDestroy(Symbol->dt);

    free(Symbol->label);
    free(Symbol);
}

sym* symCreateScope (sym* Parent) {
    return symCreateParented(symScope, Parent);
}

sym* symCreateModuleLink (sym* parent, const sym* module) {
    sym* Symbol = symCreateParented(symModuleLink, parent);
    vectorPush(&Symbol->children, (sym*) module);
    return Symbol;
}

static sym* symCreateLink (sym* Symbol) {
    sym* Link = symCreate(symLink);
    vectorPush(&Link->children, (sym*) Symbol);
    return Link;
}

sym* symCreateType (sym* Parent, const char* ident, int size, symTypeMask typeMask) {
    sym* Symbol = symCreateParented(symType, Parent);
    Symbol->ident = strdup(ident);
    Symbol->size = size;
    Symbol->typeMask = typeMask;
    Symbol->complete = true;
    return Symbol;
}

sym* symCreateNamed (symTag tag, sym* Parent, const char* ident) {
    sym* Symbol = symCreateParented(tag, Parent);
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
    Child->parent = Parent;
    Child->nthChild = vectorPush(&Parent->children, Child);
}

void symChangeParent (sym* Symbol, sym* parent) {
    /*Replace it in the old parent's vector with a link*/
    vectorSet(&Symbol->parent->children, Symbol->nthChild,
              symCreateLink(Symbol));

    /*Add it to the new parent*/
    symAddChild(parent, Symbol);
}

const sym* symGetNthParam (const sym* fn, int n) {
    int paramNo = 0;

    for (int i = 0; i < fn->children.length; i++) {
        const sym* child = vectorGet(&fn->children, i);

        if (child->tag == symParam)
            if (paramNo++ == n)
                return child;
    }

    return 0;
}

sym* symChild (const sym* Scope, const char* look) {
    //printf("searching: %s\n", Scope->ident);

    for (int n = 0; n < Scope->children.length; n++) {
        sym* Current = vectorGet(&Scope->children, n);
        //reportSymbol(Current);
        //getchar();

        /*Found it?*/
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

        /*Included module?*/
        if (Current->tag == symModuleLink) {
            sym* Found = symChild(Current->children.buffer[0], look);

            if (Found)
                return Found;

        /*Reparented symbol?*/
        } else if (Current->tag == symLink) {
            sym* Found = symChild(Current, look);

            if (Found)
                return Found;
        }
    }

    return 0;
}

sym* symFind (const sym* Scope, const char* look) {
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
    else if (tag == symModuleLink) return "module link";
    else if (tag == symLink) return "sym link";
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
