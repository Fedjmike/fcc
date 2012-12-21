struct ast;
struct sym;

/**
 * Semantic analyzer context
 *
 * Used only by internal analyzerXXX functions
 */
typedef struct {
    ///An array of built-in types
    struct sym** types;

    ///Return type of the function currently in
    type returnType;

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
 * Assumes a well formed AST in terms of fields filled and constraints on
 * what fills them upheld. For example, the children of a value class are
 * assumed to all be value classes themselves. The o string in a BOP is
 * assumed to be a valid operator. A full description of these constraints
 * is in ast.h.
 */
analyzerResult analyzer (struct ast* Tree, struct sym* Global, struct sym** Types);

void analyzerErrorExpected (analyzerCtx* ctx, struct ast* Node, char* where, char* Expected, type Found);

void analyzerErrorExpectedType (analyzerCtx* ctx, struct ast* Node, char* where, type Expected, type Found);

void analyzerErrorOp (analyzerCtx* ctx, struct ast* Operator, char* o, char* desc, struct  ast* Operand, type DT);

void analyzerErrorMismatch (analyzerCtx* ctx, struct ast* Node, char* o, type L, type R);

void analyzerErrorDegree (analyzerCtx* ctx, struct ast* Node, char* thing, int expected, int found, char* where);

void analyzerErrorParamMismatch (analyzerCtx* ctx, struct ast* Node, int n, type Expected, type Found);
void analyzerWarningParamMismatch (analyzerCtx* ctx, struct ast* Node, int n, type Expected, type Found);
