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

void errorRedeclaredSymAs (parserCtx* ctx, const sym* Symbol, symTag tag) {
    const ast* first = (const ast*) vectorGet(&Symbol->decls, 0);

    parserError(ctx, "'%s' redeclared as %s", Symbol->ident, symTagGetStr(tag));

    tokenLocationMsg(first->location);
    printf("first declaration here as %s\n", symTagGetStr(Symbol->tag));
}

void errorReimplementedSym (parserCtx* ctx, const sym* Symbol) {
    parserError(ctx, "%s '%s' reimplemented",
          Symbol->tag == symId ? "function" : symTagGetStr(Symbol->tag), Symbol->ident);

    tokenLocationMsg(Symbol->impl->location);
    puts("first implementation here");
}

void errorFileNotFound (parserCtx* ctx, const char* name) {
    parserError(ctx, "file not found, '%s'", name);
}

/*:::: ANALYZER ERRORS ::::*/

void errorTypeExpected (analyzerCtx* ctx, const ast* Node, const char* where, const char* expected) {
    char* found = typeToStr(Node->dt, "");
    analyzerError(ctx, Node, "%s requires %s, found '%s'", where, expected, found);
    free(found);
}

void errorTypeExpectedType (analyzerCtx* ctx, const ast* Node, const char* where, const type* expected) {
    char* expectedStr = typeToStr(expected, "");
    errorTypeExpected(ctx, Node, where, expectedStr);
    free(expectedStr);
}

void errorLvalue (analyzerCtx* ctx, const ast* Node, const char* o) {
    analyzerError(ctx, Node, "%s requires lvalue", o);
}

void errorMismatch (analyzerCtx* ctx, const ast* Node, const char* o) {
    char* L = typeToStr(Node->l->dt, "");
    char* R = typeToStr(Node->r->dt, "");
    analyzerError(ctx, Node, "type mismatch between %s and %s for %s", L, R, o);
    free(L);
    free(R);
}

void errorDegree (analyzerCtx* ctx, const ast* Node,
                  const char* thing, int expected, int found, const char* where) {
    analyzerError(ctx, Node, "%s expected %d %s, %d given", where, expected, thing, found);
}

void errorParamMismatch (analyzerCtx* ctx, const ast* Node,
                         int n, const type* expected, const type* found) {
    char* expectedStr = typeToStr(expected, "");
    char* foundStr = typeToStr(found, "");
    analyzerError(ctx, Node, "type mismatch at parameter %d, %s: found %s",
                  n+1, expectedStr, foundStr);
    free(expectedStr);
    free(foundStr);
}

void errorNamedParamMismatch (analyzerCtx* ctx, const ast* Node,
                              int n, const sym* fn, const type* found) {
    char* foundStr = typeToStr(found, "");
    const sym* param = symGetChild(fn, n);
    char* paramStr = typeToStr(param->dt, param->ident ? param->ident : "");
    analyzerError(ctx, Node, "type mismatch at parameter %d of %s, %s: found %s",
                  n+1, fn->ident, paramStr, foundStr);
    free(foundStr);
    free(paramStr);
}

void errorMember (analyzerCtx* ctx, const char* o, const ast* Node, const type* record) {
    char* recordStr = typeToStr(record, "");
    analyzerError(ctx, Node, "%s expected field of %s, found %s", o, recordStr, Node->literal);
    free(recordStr);
}

void errorInitFieldMismatch (analyzerCtx* ctx, const ast* Node,
                             const sym* structSym, const sym* fieldSym) {
    char* fieldStr = typeToStr(fieldSym->dt, fieldSym->ident);
    char* foundStr = typeToStr(Node->dt,   (Node->symbol && Node->symbol->ident)
                                         ? Node->symbol->ident : "");
    analyzerError(ctx, Node, "type mismatch: %s given for initialization of field %s in %s %s",
                  foundStr, fieldStr, symTagGetStr(structSym->tag), structSym->ident);
    free(fieldStr);
    free(foundStr);
}

void errorConflictingDeclarations (analyzerCtx* ctx, const ast* Node,
                                   const sym* Symbol, const type* found) {
    char* expectedStr = typeToStr(Symbol->dt, Symbol->ident);
    char* foundStr = typeToStr(found, "");
    analyzerError(ctx, Node, "%s redeclared as conflicting type %s", expectedStr, foundStr);
    free(expectedStr);
    free(foundStr);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (Current->location.line != Node->location.line) {
            tokenLocationMsg(Current->location);
            printf("also declared here\n");
        }
    }
}

void errorRedeclared (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    if (Symbol->tag == symId || Symbol->tag == symParam) {
        char* symStr = typeToStr(Symbol->dt, Symbol->ident);
        analyzerError(ctx, Node, "%s redeclared", symStr);
        free(symStr);

    } else
        analyzerError(ctx, Node, "%s %s redeclared", symTagGetStr(Symbol->tag), Symbol->ident);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (   Current->location.line != Node->location.line
            || Current->location.filename != Node->location.filename) {
            tokenLocationMsg(Current->location);
            printf("also declared here\n");
        }
    }
}

void errorIllegalSymAsValue (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    char* SymStr = typeToStr(Symbol->dt, Symbol->ident);
    analyzerError(ctx, Node, "cannot use %s %s as a value", symTagGetStr(Symbol->tag), SymStr);
    free(SymStr);
}

void errorCompileTimeKnown (analyzerCtx* ctx, const ast* Node, const sym* Symbol, const char* what) {
    char* SymStr = typeToStr(Symbol->dt, Symbol->ident);
    analyzerError(ctx, Node, "declaration of %s needed a compile-time known %s", SymStr, what);
    free(SymStr);
}

void errorCompoundLiteralWithoutType (struct analyzerCtx* ctx, const struct ast* Node) {
    analyzerError(ctx, Node, "compound literal without explicit type");
}
