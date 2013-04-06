#include "../inc/analyzer.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"

#include "../inc/analyzer-value.h"
#include "../inc/analyzer-decl.h"

#include "stdlib.h"
#include "stdarg.h"

static void analyzerModule (analyzerCtx* ctx, ast* Node);

static void analyzerFnImpl (analyzerCtx* ctx, ast* Node);
static void analyzerCode (analyzerCtx* ctx, ast* Node);
static void analyzerBranch (analyzerCtx* ctx, ast* Node);
static void analyzerLoop (analyzerCtx* ctx, ast* Node);
static void analyzerIter (analyzerCtx* ctx, ast* Node);
static void analyzerReturn (analyzerCtx* ctx, ast* Node);

static void analyzerError (analyzerCtx* ctx, const ast* Node, const char* format, ...) {
    printf("error(%d:%d): ", Node->location.line, Node->location.lineChar);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;

    debugWait();
}

void analyzerErrorExpected (analyzerCtx* ctx, const ast* Node, const char* where, const char* Expected, const type* Found) {
    char* FoundStr = typeToStr(Found, "");

    analyzerError(ctx, Node, "%s expected %s, found %s",
                  where, Expected, FoundStr);

    free(FoundStr);
}

void analyzerErrorExpectedType (analyzerCtx* ctx, const ast* Node, const char* where, const type* Expected, const type* Found) {
    char* ExpectedStr = typeToStr(Expected, "");

    analyzerErrorExpected(ctx, Node, where, ExpectedStr, Found);

    free(ExpectedStr);
}

void analyzerErrorOp (analyzerCtx* ctx, const char* o, const char* desc, const ast* Operand, const type* DT) {
    char* DTStr = typeToStr(DT, "");

    analyzerError(ctx, Operand, "%s requires %s, found %s",
                  o, desc, DTStr);

    free(DTStr);
}

void analyzerErrorMismatch (analyzerCtx* ctx, const ast* Node, const char* o, const type* L, const type* R) {
    char* LStr = typeToStr(L, "");
    char* RStr = typeToStr(R, "");

    analyzerError(ctx, Node, "type mismatch between %s and %s for %s",
                  LStr, RStr, o);

    free(LStr);
    free(RStr);
}

void analyzerErrorDegree (analyzerCtx* ctx, const ast* Node, const char* thing, int expected, int found, const char* where) {
    analyzerError(ctx, Node, "%s expected %d %s, %d given",
                  where, expected, thing, found);
}

void analyzerErrorParamMismatch (analyzerCtx* ctx, const ast* Node, int n, const type* Expected, const type* Found) {
    char* ExpectedStr = typeToStr(Expected, "");
    char* FoundStr = typeToStr(Found, "");

    analyzerError(ctx, Node, "type mismatch at parameter %d: expected %s, found %s",
                  n+1, ExpectedStr, FoundStr);

    free(ExpectedStr);
    free(FoundStr);
}

void analyzerErrorMember (analyzerCtx* ctx, const char* o, const ast* Node, const type* Record) {
    char* RecordStr = typeToStr(Record, "");

    analyzerError(ctx, Node, "%s expected field of %s, found %s",
                  o, RecordStr, Node->literal);

    free(RecordStr);
}

void errorConflictingDeclarations (analyzerCtx* ctx, const ast* Node, const sym* Symbol, const type* Found) {
    char* ExpectedStr = typeToStr(Symbol->dt, Symbol->ident);
    char* FoundStr = typeToStr(Found, "");

    analyzerError(ctx, Node, "%s redeclared as conflicting type %s",
                  ExpectedStr, FoundStr);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (ast*) Symbol->decls.buffer[n];

        printf("     (%d:%d): also declared here\n",
               Current->location.line, Current->location.lineChar);
    }

    free(FoundStr);
}

static analyzerCtx* analyzerInit (sym** Types) {
    analyzerCtx* ctx = malloc(sizeof(analyzerCtx));
    ctx->types = Types;

    ctx->errors = 0;
    ctx->warnings = 0;
    return ctx;
}

static void analyzerEnd (analyzerCtx* ctx) {
    free(ctx);
}

analyzerResult analyzer (ast* Tree, sym** Types) {
    analyzerCtx* ctx = analyzerInit(Types);

    analyzerModule(ctx, Tree);
    analyzerResult result = {ctx->errors, ctx->warnings};

    analyzerEnd(ctx);
    return result;
}

static void analyzerModule (analyzerCtx* ctx, ast* Node) {
    debugEnter("Module");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerNode(ctx, Current);
        //debugWait();
    }

    debugLeave();
}

void analyzerNode (analyzerCtx* ctx, ast* Node) {
    if (Node->tag == astEmpty)
        debugMsg("Empty");

    else if (Node->tag == astInvalid)
        debugMsg("Invalid");

    else if (Node->tag == astFnImpl)
        analyzerFnImpl(ctx, Node);

    else if (Node->tag == astDeclStruct)
        analyzerDeclStruct(ctx, Node);

    else if (Node->tag == astDecl)
        analyzerDecl(ctx, Node);

    else if (Node->tag == astCode)
        analyzerCode(ctx, Node);

    else if (Node->tag == astBranch)
        analyzerBranch(ctx, Node);

    else if (Node->tag == astLoop)
        analyzerLoop(ctx, Node);

    else if (Node->tag == astIter)
        analyzerIter(ctx, Node);

    else if (Node->tag == astReturn)
        analyzerReturn(ctx, Node);

    else if (Node->tag == astBreak)
        ; /*Nothing to check (inside breakable block is a parsing issue)*/

    else if (astIsValueTag(Node->tag))
        /*TODO: Check not throwing away value*/
        analyzerValue(ctx, Node);

    else
        debugErrorUnhandled("analyzerNode", "AST tag", astTagGetStr(Node->tag));
}

static void analyzerFnImpl (analyzerCtx* ctx, ast* Node) {
    debugEnter("FnImpl");

    analyzerDecl(ctx, Node->l);

    ctx->returnType = Node->symbol->dt->returnType;
    analyzerNode(ctx, Node->r);
    ctx->returnType = 0;

    debugLeave();
}

static void analyzerCode (analyzerCtx* ctx, ast* Node) {
    debugEnter("Code");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        analyzerNode(ctx, Current);

    debugLeave();
}

static void analyzerBranch (analyzerCtx* ctx, ast* Node) {
    debugEnter("Branch");

    /*Is the condition a valid condition?*/

    ast* cond = Node->firstChild;
    const type* condDT = analyzerValue(ctx, cond);

    if (!typeIsCondition(condDT))
        analyzerErrorExpected(ctx, cond, "if", "condition", condDT);

    /*Code*/

    analyzerNode(ctx, Node->l);

    if (Node->r)
        analyzerNode(ctx, Node->r);

    debugLeave();
}

static void analyzerLoop (analyzerCtx* ctx, ast* Node) {
    debugEnter("Loop");

    /*do while?*/
    bool isDo = Node->l->tag == astCode;
    ast* cond = isDo ? Node->r : Node->l;
    ast* code = isDo ? Node->l : Node->r;

    /*Condition*/

    const type* condDT = analyzerValue(ctx, cond);

    if (!typeIsCondition(condDT))
        analyzerErrorExpected(ctx, cond, "do loop", "condition", condDT);

    /*Code*/

    analyzerNode(ctx, code);

    debugLeave();
}

static void analyzerIter (analyzerCtx* ctx, ast* Node) {
    debugEnter("Iter");

    ast* init = Node->firstChild;
    ast* cond = init->nextSibling;
    ast* iter = cond->nextSibling;

    /*Initializer*/

    if (init->tag == astDecl)
        analyzerNode(ctx, init);

    else if (init->tag != astEmpty)
        analyzerValue(ctx, init);

    /*Condition*/

    if (cond->tag != astEmpty) {
        const type* condDT = analyzerValue(ctx, cond);

        if (!typeIsCondition(condDT))
            analyzerErrorExpected(ctx, cond, "for loop", "condition", condDT);
    }

    /*Iterator*/

    if (iter->tag != astEmpty)
        analyzerValue(ctx, iter);

    /*Code*/

    analyzerNode(ctx, Node->l);

    debugLeave();
}

static void analyzerReturn (analyzerCtx* ctx, ast* Node) {
    debugEnter("Return");

    /*Return type, if any, matches?*/

    const type* R = typeCreateInvalid();

    if (Node->r)
        R = analyzerValue(ctx, Node->r);

    if (!typeIsCompatible(R, ctx->returnType))
        analyzerErrorExpectedType(ctx, Node->r, "return", ctx->returnType, R);

    debugLeave();
}

