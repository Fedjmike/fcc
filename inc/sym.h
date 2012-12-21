#pragma once

#include "type.h"
#include "operand.h"

/**
 * Indices of certain built in symbols for arrays passed between
 * main and the semantic analyzer.
 * @see main @see analyzer
 */
typedef enum {
    builtinVoid,
    builtinBool,
    builtinChar,
    builtinInt
} builtinTypeIndex;

/**
 * Symbol classes
 * @see sym @see sym::class
 */
typedef enum {
    symUndefined,
    symScope,
    symType,
    symStruct,
    symEnum,
    symFunction,
    symParam,
    symVar
} symClass;

/**
 * Storage classes for a symbol, defining their linkage and lifetime
 */
typedef enum {
    storageUndefined,
    storageAuto,
    storageRegister,
    storageStatic,
    storageExtern
} storageClass;

/**
 * Bitmask attributes for types and structs defining their operations
 * @see sym::typeMask
 */
typedef enum {
    ///Numeric describes whether arithmetic operators can be performed
    ///on it. e.g. +, unary -, bitwise &
    typeNumeric    = 1 << 0,
    ///Ordinal describes whether it has a defined order, and therefore
    ///can be compared with <, <=, >, >=
    typeOrdinal    = 1 << 1,
    ///Equality describes whether equality can be tested with != and ==
    typeEquality   = 1 << 2,
    ///Assignment describes whether you can assign new values directly
    ///with =
    typeAssignment = 1 << 3,
    ///Combination of attributes for integral types
    typeIntegral = typeNumeric || typeOrdinal || typeEquality || typeAssignment
} symTypeMask;

/**
 * A member of the symbol table.
 *
 * Can represent functions, structs, parameters, unnamed scopes etc
 *
 * @see symInit @see symEnd
 */
typedef struct sym {
    symClass class;
    char* ident;

    bool proto;  ///Has only a prototype been declared so far?

    /*Functions, params, vars only*/
    storageClass storage;
    type dt;  ///In the case of functions, the return type

    /*Types and structs only*/
    int size;  ///Size in bytes
    symTypeMask typeMask;  ///A mask defining operator capabilities

    /*Linked list of symbols in our namespace
      Including parameters for functions and constants in enums*/
    struct sym* parent;
    struct sym* firstChild;
    struct sym* lastChild;
    struct sym* nextSibling;

    int params;  ///Number of parameters, updated when params added to namespace

    operand label;  ///Label associated with this symbol in the assembly
    int offset;  ///Offset, in bytes, for stack stored vars/parameters and non
                 ///static fields
} sym;

/**
 * Initialize the symbol table
 * @return the global namespace symbol
 * @see symEnd
 */
sym* symInit ();

/**
 * Destroy a symbol table
 */
void symEnd (sym* Global);

sym* symCreate (symClass class, sym* Parent);
sym* symCreateType (sym* Parent, char* ident, int size, symTypeMask typeMask);
sym* symCreateStruct (sym* Parent, char* ident);
sym* symCreateVar (sym* Parent, char* ident, type DT, storageClass storage);

/**
 * Attempt to find a symbol directly accessible from a scope. Will search
 * inside contained enums, anon. unions but will not look at parent scopes.
 * @return Symbol, or null on failure.
 */
sym* symChild (sym* Scope, char* Look);

/**
 * Attempt to find a symbol visible from a scope. Will recurse up parent
 * scopes.
 * @return Symbol, or null on failure
 */
sym* symFind (sym* Scope, char* Look);

const char* symClassGetStr (symClass class);
const char* storageClassGetStr (storageClass class);
