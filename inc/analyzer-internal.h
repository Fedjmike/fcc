#include "../std/std.h"

#include "hashmap.h"

typedef struct ast ast;
typedef struct sym sym;
typedef struct type type;
typedef struct architecture architecture;

/**
 * Analyzer context specific to a function
 */
typedef struct analyzerFnCtx {
    sym* fn;
    type* returnType;
} analyzerFnCtx;

/**
 * Semantic analyzer context
 *
 * Used only by internal analyzerXXX functions
 */
typedef struct analyzerCtx {
    ///An array of built-in types
    sym** types;
    ///Architecture, used only for compile time evaluation of array sizes and,
    ///in turn, enum constants
    const architecture* arch;

    ///Current function context
    analyzerFnCtx fnctx;

    ///Incomplete types for which an incomplete decl or dereference error has
    ///already been issued, and subsequent errors are to be suppressed
    intset/*<const sym*>*/ incompleteDeclIgnore;
    intset/*<const sym*>*/ incompletePtrIgnore;

    int errors;
    int warnings;
} analyzerCtx;

/*==== analyzer.c ====*/

/**
 * Handles any node tag by passing it off to one of the
 * specialized handlers.
 */
void analyzerNode (analyzerCtx* ctx, ast* Node);

/*==== analyzer-value.c ====*/

const type* analyzerValue (analyzerCtx* ctx, ast* Node);
void analyzerCompoundInit (analyzerCtx* ctx, ast* Node, const type* DT);

/*==== analyzer-decl.c ====*/

void analyzerDecl (analyzerCtx* ctx, ast* Node, bool module);
const type* analyzerType (analyzerCtx* ctx, ast* Node);
type* analyzerParamList (analyzerCtx* ctx, ast* Node, type* returnType);
