#pragma once

#include "vector.h"

typedef struct type type;
typedef struct ast ast;
typedef struct sym sym;

typedef struct irFn irFn;

/**
 * Symbol tags
 * @see sym @see sym::tag
 */
typedef enum symTag {
    symUndefined,
    symScope,
    ///When a module is included, a module link is added to the current scope.
    ///Its first child is the module scope of the file included. symFind and
    ///symChild will look in here for symbols.
    symModuleLink,
    ///Whenever a symbol is (legally) redeclared, it is replaced by a link in
    ///its original scope, and the symbol moved to the new scope. This is so
    ///a function implementation sees the symbols of the scope it is actually
    ///in, not of the first declaration. symFind and symChild work as above.
    ///@see symChangeParent
    symLink,
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
    typeBool = typeEquality | typeAssignment | typeCondition,
    typeStruct = typeAssignment,
    typeUnion = typeAssignment,
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

    ///Children, including parameters for functions and constants in enums
    vector/*<sym*>*/ children;
    sym* parent;
    ///Position in parent's vector
    int nthChild;

    ///For functions
    irFn* ir;
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
void symEnd (sym* global);

sym* symCreateScope (sym* parent);
sym* symCreateModuleLink (sym* parent, const sym* module);
sym* symCreateType (sym* parent, const char* ident, int size, symTypeMask typeMask);
sym* symCreateNamed (symTag tag, sym* Parent, const char* ident);

/**
 * Changes the parent of a symbol, replacing it with a symLink
 */
void symChangeParent (sym* Symbol, sym* parent);

/**
 * Attempt to find a symbol directly accessible from a scope. Will search
 * inside contained enums, anon. unions but will not look at parent scopes.
 * @return Symbol, or null on failure.
 */
sym* symChild (const sym* scope, const char* look);

/**
 * Attempt to find a symbol visible from a scope. Will recurse up parent
 * scopes.
 * @return Symbol, or null on failure
 */
sym* symFind (const sym* scope, const char* look);

const char* symTagGetStr (symTag tag);
const char* storageTagGetStr (storageTag tag);
