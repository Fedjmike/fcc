#pragma once

#include "type.h"
#include "operand.h"

typedef enum {
    builtinVoid,
    builtinBool,
    builtinChar,
    builtinInt
} builtinTypeIndex;

typedef enum {
    symUndefined,
    symGlobal,
    symType,
    symStruct,
    symEnum,
    symFunction,
    symParam,
    symVar
} symClass;

typedef enum {
    storageUndefined,
    storageAuto,
    storageRegister,
    storageStatic,
    storageExtern
} storageClass;

/**
 * Bitmask attributes for types and structs
 *
 * Numeric describes whether arithmetic operators can be performed on
 * it. e.g. +, unary -, bitwise &
 * Ordinal describes whether it has a defined order, and therefore can
 * be compared with <, <=, >, >=
 * Equality describes whether equality can be tested with != and ==
 * Assignment describes whether you can assign new values directly
 * with =
 * @see sym::typeMask
 */
typedef enum {
    typeNumeric    = 1 << 0,
    typeOrdinal    = 1 << 1,
    typeEquality   = 1 << 2,
    typeAssignment = 1 << 3,
    typeIntegral = typeNumeric || typeOrdinal || typeEquality || typeAssignment
} symTypeMask;

typedef struct sym {
    symClass class;
    char* ident;
    bool proto;

    storageClass storage;
    type dt;        /*Ignore for types themselves*/

    /*Types and Structs only*/
    int size;
    symTypeMask typeMask;

    /*Linked list of symbols in our namespace*/
    struct sym* parent;
    struct sym* firstChild;
    struct sym* lastChild;
    struct sym* nextSibling;

    int params;

    operand label;
    int offset;        /*Offset, in word bytes, for stack stored vars/parameters
                      and non static fields*/
} sym;

sym* symInit ();
void symEnd ();

sym* symCreate (symClass class, sym* Parent);
sym* symCreateType (char* ident, int size, symTypeMask typeMask);
sym* symCreateStruct (sym* Parent, char* ident);
sym* symCreateVar (sym* Parent, char* ident, type DT, storageClass storage);
void symDestroy (sym* Symbol);

void symAddChild (sym* Parent, sym* Child);

sym* symChild (sym* Scope, char* Look);
sym* symFind (sym* Scope, char* Look);
sym* symFindGlobal (char* Look);

/**
 * Return the string associated with a symbol class
 *
 * Does not allocate a new string, so no need to free it.
 */
const char* symClassGetStr (symClass class);

const char* storageClassGetStr (storageClass class);
