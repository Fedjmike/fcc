#pragma once

#include "../std/std.h"

#include "vector.h"

#include "operand.h"

using "vector.h";

using "operand.h";

typedef struct type type;
typedef struct ast ast;
typedef struct sym sym;

/**
 * Symbol tags
 * @see sym @see sym::tag
 */
typedef enum symTag {
    symUndefined,
    symScope,
    symType,
    symTypedef,
    symStruct,
    symUnion,
    symEnum,
    symEnumConstant,
    symId,
    symParam
} symTag;

/**
 * Storage tags for a symbol, defining their linkage and lifetime
 */
typedef enum storageTag {
    storageUndefined,
    storageAuto,
    storageStatic,
    storageExtern,
    storageTypedef
} storageTag;

/**
 * Bitmask attributes for types and structs defining their operations
 * @see sym::typeMask
 */
typedef enum symTypeMask {
    typeNone,
    ///Numeric describes whether arithmetic operators can be performed
    ///on it. e.g. +, unary -, bitwise &
    typeNumeric = 1 << 0,
    ///Ordinal describes whether it has a defined order, and therefore
    ///can be compared with <, <=, >, >=
    typeOrdinal = 1 << 1,
    ///Equality describes whether equality can be tested with != and ==
    typeEquality = 1 << 2,
    ///Assignment describes whether you can assign new values directly
    ///with =
    typeAssignment = 1 << 3,
    ///Condition describes whether the type can be tested for boolean
    ///truth
    typeCondition = 1 << 4,
    ///Combination of attributes
    typeIntegral = typeNumeric | typeOrdinal | typeEquality | typeAssignment | typeCondition,
    typeStruct = typeAssignment,
    typeEnum = typeIntegral
} symTypeMask;

/**
 * A member of the symbol table.
 *
 * Can represent functions, structs, parameters, unnamed scopes etc
 *
 * @see symInit @see symEnd
 */
typedef struct sym {
    symTag tag;
    char* ident;

    ///Vector of AST nodes for each declaration (inc. impls)
    vector/*<const ast* >*/ decls;
    ///Implementation
    ///Points to the astFnImpl, astStruct etc, whichever relevant if any
    const ast* impl;

    /*Functions, params, vars only*/
    storageTag storage;
    ///In the case of functions, the return type
    type* dt;

    /*Types and structs only*/
    ///Size in bytes
    int size;
    ///A mask defining operator capabilities
    symTypeMask typeMask;
    bool complete;

    /*Linked list of symbols in our namespace
      Including parameters for functions and constants in enums*/
    sym* parent;
    sym* firstChild;
    sym* lastChild;
    sym* nextSibling;
    int children;

    ///Label associated with this symbol in the assembly
    char* label;
    ///Offset, in bytes, for stack stored vars/parameters and non
    ///static fields
    int offset;
    ///For enum constants
    int constValue;
} sym;

/**
 * Initialize the symbol table
 * @return the global namespace symbol
 * @see symEnd
 */
sym* symInit (void);

/**
 * Destroy a symbol table
 */
void symEnd (sym* Global);

sym* symCreateScope (sym* Parent);
sym* symCreateType (sym* Parent, const char* ident, int size, symTypeMask typeMask);
sym* symCreateNamed (symTag tag, sym* Parent, const char* ident);

/**
 * Get the nth child of a symbol, returning 0 if there are fewer
 */
const sym* symGetChild (const sym* Parent, int n);

/**
 * Attempt to find a symbol directly accessible from a scope. Will search
 * inside contained enums, anon. unions but will not look at parent scopes.
 * @return Symbol, or null on failure.
 */
sym* symChild (const sym* Scope, const char* look);

/**
 * Attempt to find a symbol visible from a scope. Will recurse up parent
 * scopes.
 * @return Symbol, or null on failure
 */
sym* symFind (const sym* Scope, const char* look);

const char* symTagGetStr (symTag tag);
const char* storageTagGetStr (storageTag tag);
