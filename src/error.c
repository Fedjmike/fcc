#include "../inc/error.h"

#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/debug.h"

#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/analyzer.h"

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"

static void tokenLocationMsg (tokenLocation loc) {
    printf("%s:%d:%d: ", loc.filename, loc.line, loc.lineChar);
}

static void parserError (parserCtx* ctx, const char* format, ...) {
    tokenLocationMsg(ctx->location);
    printf("error: ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;
    debugWait();
}

static void analyzerError (analyzerCtx* ctx, const ast* Node, const char* format, ...) {
    tokenLocationMsg(Node->location);
    printf("error: ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;
    debugWait();
}

/*:::: PARSER ERRORS ::::*/

void errorExpected (parserCtx* ctx, const char* expected) {
    parserError(ctx, "expected %s, found '%s'", expected, ctx->lexer->buffer);
}

void errorUndefSym (parserCtx* ctx) {
    parserError(ctx, "undefined symbol '%s'", ctx->lexer->buffer);
}

void errorUndefType (parserCtx* ctx) {
    parserError(ctx, "undefined symbol '%s', expected type", ctx->lexer->buffer);
}

void errorIllegalOutside (parserCtx* ctx, const char* what, const char* where) {
    parserError(ctx, "illegal %s outside of %s", what, where);
}

void errorRedeclaredSymAs (parserCtx* ctx, sym* Symbol, symTag tag) {
    const ast* first = (const ast*) vectorGet(&Symbol->decls, 0);

    parserError(ctx, "'%s' redeclared as %s", Symbol->ident, symTagGetStr(tag));

    tokenLocationMsg(first->location);
    printf("       first declaration here as %s\n", symTagGetStr(Symbol->tag));
}

void errorReimplementedSym (parserCtx* ctx, sym* Symbol) {
    parserError(ctx, "%s '%s' reimplemented",
          Symbol->tag == symId ? "function" : symTagGetStr(Symbol->tag), Symbol->ident);

    tokenLocationMsg(Symbol->impl->location);
    puts("       first implementation here");
}

void errorFileNotFound (parserCtx* ctx, const char* name) {
    parserError(ctx, "File not found, '%s'", name);
}

/*:::: ANALYZER ERRORS ::::*/

void errorTypeExpected (analyzerCtx* ctx, const ast* Node, const char* where, const char* expected, const type* found) {
    char* foundStr = typeToStr(found, "");
    analyzerError(ctx, Node, "%s expected %s, found %s", where, expected, foundStr);
    free(foundStr);
}

void errorTypeExpectedType (analyzerCtx* ctx, const ast* Node, const char* where, const type* expected, const type* found) {
    char* expectedStr = typeToStr(expected, "");
    errorTypeExpected(ctx, Node, where, expectedStr, found);
    free(expectedStr);
}

void errorOp (analyzerCtx* ctx, const char* o, const char* desc, const ast* Operand, const type* DT) {
    char* DTStr = typeToStr(DT, "");
    analyzerError(ctx, Operand, "%s requires %s, found %s", o, desc, DTStr);
    free(DTStr);
}

void errorLvalue (analyzerCtx* ctx, const char* o, const struct ast* Operand) {
    analyzerError(ctx, Operand, "%s requires lvalue", o);
}

void errorMismatch (analyzerCtx* ctx, const ast* Node, const char* o, const type* L, const type* R) {
    char* LStr = typeToStr(L, "");
    char* RStr = typeToStr(R, "");
    analyzerError(ctx, Node, "type mismatch between %s and %s for %s", LStr, RStr, o);
    free(LStr);
    free(RStr);
}

void errorDegree (analyzerCtx* ctx, const ast* Node, const char* thing, int expected, int found, const char* where) {
    analyzerError(ctx, Node, "%s expected %d %s, %d given", where, expected, thing, found);
}

void errorParamMismatch (analyzerCtx* ctx, const ast* Node, int n, const type* Expected, const type* Found) {
    char* ExpectedStr = typeToStr(Expected, "");
    char* FoundStr = typeToStr(Found, "");
    analyzerError(ctx, Node, "type mismatch at parameter %d: expected %s, found %s", n+1, ExpectedStr, FoundStr);
    free(ExpectedStr);
    free(FoundStr);
}

void errorMember (analyzerCtx* ctx, const char* o, const ast* Node, const type* Record) {
    char* RecordStr = typeToStr(Record, "");
    analyzerError(ctx, Node, "%s expected field of %s, found %s", o, RecordStr, Node->literal);
    free(RecordStr);
}

void errorConflictingDeclarations (analyzerCtx* ctx, const ast* Node, const sym* Symbol, const type* found) {
    char* expectedStr = typeToStr(Symbol->dt, Symbol->ident);
    char* foundStr = typeToStr(found, "");
    analyzerError(ctx, Node, "%s redeclared as conflicting type %s", expectedStr, foundStr);
    free(expectedStr);
    free(foundStr);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (Current->location.line != Node->location.line) {
            tokenLocationMsg(Current->location);
            printf("       also declared here\n");
        }
    }
}

void errorRedeclaredVar (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    char* symStr = typeToStr(Symbol->dt, Symbol->ident);
    analyzerError(ctx, Node, "%s redeclared", symStr);
    free(symStr);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (Current->location.line != Node->location.line)
            tokenLocationMsg(Current->location);
            printf("       also declared here\n");
    }
}

void errorIllegalSymAsValue (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    analyzerError(ctx, Node, "cannot use a %s as a value", symTagGetStr(Symbol->tag));
}
