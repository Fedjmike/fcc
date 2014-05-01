#include "hashmap.h"

using "forward.h";

using "hashmap.h";

typedef struct ast ast;
typedef struct sym sym;
typedef struct type type;
typedef struct architecture architecture;

/**
 * Analyzer context specific to a function
 */
typedef struct analyzerFnCtx {
    sym* fn;
    ///Return type
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

/**
 * Semantic analysis result
 */
typedef struct analyzerResult {
    int errors;
    int warnings;
} analyzerResult;

/**
 * Analyze the semantics of an AST tree in the context of a symbol table
 *
 * Attaches types to AST nodes and symbols and validates their semantics.
 *
 * Assumes a well formed AST in terms of fields filled and constraints on
 * what fills them upheld.
 */
analyzerResult analyzer (ast* Tree, sym** Types, const architecture* arch);

/**
 * Handles any node tag by passing it off to one of the following
 * specialized handlers.
 */
void analyzerNode (analyzerCtx* ctx, ast* Node);
