struct ast;
struct sym;

/**
 * Analyzer context specific to a function
 */
typedef struct analyzerFnCtx {
    struct sym* fn;
    ///Return type
    struct type* returnType;
} analyzerFnCtx;

/**
 * Semantic analyzer context
 *
 * Used only by internal analyzerXXX functions
 */
typedef struct analyzerCtx {
    ///An array of built-in types
    struct sym** types;

    ///Current function context
    analyzerFnCtx fnctx;

    int errors;
    int warnings;
} analyzerCtx;

/**
 * Semantic analysis result
 */
typedef struct {
    int errors;
    int warnings;
} analyzerResult;

/**
 * Analyze the semantics of an AST tree in the context of a symbol table
 *
 * Attaches types to AST nodes and symbols and validates their semantics.
 *
 * Assumes a well formed AST in terms of fields filled and constraints on
 * what fills them upheld. For example, the children of a value tag are
 * assumed to all be value tags themselves. The o string in a BOP is
 * assumed to be a valid operator.
 */
analyzerResult analyzer (struct ast* Tree, struct sym** Types);

/**
 * Handles any node tag by passing it off to one of the following
 * specialized handlers.
 */
void analyzerNode (analyzerCtx* ctx, struct ast* Node);
