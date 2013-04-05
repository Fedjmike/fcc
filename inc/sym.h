#pragma once

#include "operand.h"

struct type;

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
    symId,
    symParam
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
    ///Condition describes whether the type can be tested for boolean
    ///truth
    typeCondition = 1 << 4,
    ///Combination of attributes for integral types
    typeIntegral = typeNumeric | typeOrdinal | typeEquality | typeAssignment | typeCondition
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
    struct type* dt;  ///In the case of functions, the return type

    /*Types and structs only*/
    int size;  ///Size in bytes
    symTypeMask typeMask;  ///A mask defining operator capabilities

    /*Linked list of symbols in our namespace
      Including parameters for functions and constants in enums*/
    struct sym* parent;
    struct sym* firstChild;
    struct sym* lastChild;
    struct sym* nextSibling;

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

sym* symCreateType (sym* Parent, char* ident, int size, symTypeMask typeMask);
sym* symCreateStruct (sym* Parent, char* ident);
sym* symCreateId (sym* Parent, char* ident);
sym* symCreateParam (sym* Parent, char* ident);

/**
 * Attempt to find a symbol directly accessible from a scope. Will search
 * inside contained enums, anon. unions but will not look at parent scopes.
 * @return Symbol, or null on failure.
 */
sym* symChild (const sym* Scope, const char* Look);

/**
 * Attempt to find a symbol visible from a scope. Will recurse up parent
 * scopes.
 * @return Symbol, or null on failure
 */
sym* symFind (const sym* Scope, const char* Look);

const char* symClassGetStr (symClass class);
const char* storageClassGetStr (storageClass class);
