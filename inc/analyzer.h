typedef struct ast ast;
typedef struct sym sym;
typedef struct architecture architecture;

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
 * Attaches types and storage tags to AST nodes and symbols and validates
 * their semantics.
 *
 * Assumes a well formed AST in terms of fields filled and constraints on
 * what fills them upheld.
 */
analyzerResult analyzer (ast* Tree, sym** Types, const architecture* arch);
