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
    const struct type* returnType;

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

void analyzerErrorExpected (analyzerCtx* ctx, const struct ast* Node, const char* where, const char* Expected, const struct type* Found);

void analyzerErrorExpectedType (analyzerCtx* ctx, const struct ast* Node, const char* where, const struct type* Expected, const struct type* Found);

void analyzerErrorOp (analyzerCtx* ctx, const struct ast* Operator, const char* o, const char* desc, const struct ast* Operand, const struct type* DT);

void analyzerErrorMismatch (analyzerCtx* ctx, const struct ast* Node, const char* o, const struct type* L, const struct type* R);

void analyzerErrorDegree (analyzerCtx* ctx, const struct ast* Node, const char* thing, int expected, int found, const char* where);

void analyzerErrorParamMismatch (analyzerCtx* ctx, const struct ast* Node, int n, const struct type* Expected, const struct type* Found);
void analyzerWarningParamMismatch (analyzerCtx* ctx, const struct ast* Node, int n, const struct type* Expected, const struct type* Found);
