typedef struct type type;
typedef struct sym sym;
typedef struct ast ast;
typedef enum opTag opTag;
typedef enum symTag symTag;

typedef struct parserCtx parserCtx;
typedef struct analyzerCtx analyzerCtx;
typedef struct analyzerFnCtx analyzerFnCtx;

void errorExpected (parserCtx* ctx, const char* expected);
void errorUndefSym (parserCtx* ctx);
void errorUndefType (parserCtx* ctx);
void errorIllegalOutside (parserCtx* ctx, const char* what, const char* where);
void errorRedeclaredSymAs (parserCtx* ctx, const sym* Symbol, symTag tag);
void errorReimplementedSym (parserCtx* ctx, const sym* Symbol);
void errorFileNotFound (parserCtx* ctx, const char* name);

void errorTypeExpected (analyzerCtx* ctx, const ast* Node,
                        const char* where, const char* expected);
void errorOpTypeExpected (analyzerCtx* ctx, const ast* Node,
                          opTag o, const char* expected);
void errorTypeExpectedType (analyzerCtx* ctx, const ast* Node,
                            const char* where, const type* expected);
void errorLvalue (analyzerCtx* ctx, const ast* Node, opTag o);
void errorMismatch (analyzerCtx* ctx, const ast* Node, opTag o);
void errorVoidDeref (analyzerCtx* ctx, const ast* Node, opTag o);
void errorDegree (analyzerCtx* ctx, const ast* Node,
                  const char* thing, int expected, int found, const char* where);
void errorMember (analyzerCtx* ctx, const ast* Node, const char* field);
void errorParamMismatch (analyzerCtx* ctx, const ast* Node,
                         const ast* fn, int n, const type* expected, const type* found);

void errorVAxList (analyzerCtx* ctx, const ast* Node, const char* where, const char* which);
void errorVAxLvalue (analyzerCtx* ctx, const ast* Node, const char* where, const char* which);
void errorVAStartNonParam (analyzerCtx* ctx, const ast* Node);

void errorIllegalInit (analyzerCtx* ctx, const ast* Node, const char* what);
void errorInitMismatch (analyzerCtx* ctx, const ast* variable, const ast* init);
void errorInitFieldMismatch (analyzerCtx* ctx, const ast* Node,
                             const sym* structSym, const sym* fieldSym);

void errorConflictingDeclarations (analyzerCtx* ctx, const ast* Node,
                                   const sym* Symbol, const type* found);
void errorRedeclared (analyzerCtx* ctx, const ast* Node, const sym* Symbol);

void errorAlreadyConst (analyzerCtx* ctx, const ast* Node);
void errorIllegalConst (analyzerCtx* ctx, const ast* Node, const type* DT);

void errorIllegalSymAsValue (analyzerCtx* ctx, const ast* Node, const sym* Symbol);

void errorCompileTimeKnown (analyzerCtx* ctx, const ast* Node,
                            const sym* Symbol, const char* what);
void errorStaticCompileTimeKnown (analyzerCtx* ctx, const ast* Node, const sym* Symbol);
void errorIllegalArraySize (analyzerCtx* ctx, const ast* Node,
                            const sym* Symbol, int size);

void errorCompoundLiteralWithoutType (analyzerCtx* ctx, const ast* Node);

void errorIncompleteCompound (analyzerCtx* ctx, const ast* Node, const type* DT);
void errorIncompletePtr (analyzerCtx* ctx, const ast* Node, opTag o);
void errorIncompleteDecl (analyzerCtx* ctx, const ast* Node, const type* DT);
void errorIncompleteParamDecl (analyzerCtx* ctx, const ast* Node, const ast* fn, int n, const type* DT);
void errorIncompleteReturnDecl (analyzerCtx* ctx, const ast* Node, const type* dt);


void errorConstAssignment (analyzerCtx* ctx, const ast* Node, opTag o);

void errorFnTag (analyzerCtx* ctx, ast* Node);
void errorReturnType (analyzerCtx* ctx, ast* Node, analyzerFnCtx fnctx);
