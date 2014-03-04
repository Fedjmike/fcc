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
#include "string.h"

const char* consoleNormal  = "\033[1;0m";
const char* consoleRed     = "\033[1;31m";
const char* consoleGreen   = "\033[1;32m";
const char* consoleYellow  = "\033[1;33m";
const char* consoleBlue    = "\033[1;34m";
const char* consoleMagenta = "\033[1;35m";
const char* consoleCyan    = "\033[1;36m";
const char* consoleWhite   = "\033[1;37m";

static void tokenLocationMsg (tokenLocation loc) {
    printf("%s:%d:%d: ", loc.filename, loc.line, loc.lineChar);
}

static void errorHighlighted (const char* format, va_list args) {
    const char* colourString = consoleWhite;
    const char* colourNumber = consoleMagenta;
    const char* colourOp = consoleCyan;
    const char* colourType = consoleGreen;
    const char* colourIdent = consoleWhite;

    int flen = strlen(format);

    for (int i = 0; i < flen; i++) {
        if (format[i] == '$') {
            /*Regular string*/
            if (format[i+1] == 's')
                printf("%s", va_arg(args, char*));

            /*Highlighted string*/
            else if (format[i+1] == 'h')
                printf("%s%s%s", colourString, va_arg(args, char*), consoleNormal);

            /*Operator*/
            else if (format[i+1] == 'o')
                printf("%s%s%s", colourOp, va_arg(args, char*), consoleNormal);

            /*Integer*/
            else if (format[i+1] == 'd')
                printf("%s%d%s", colourNumber, va_arg(args, int), consoleNormal);

            /*Raw type*/
            else if (format[i+1] == 't') {
                char* typeStr = typeToStr(va_arg(args, type*), "");
                printf("%s%s%s", colourType, typeStr, consoleNormal);
                free(typeStr);

            /*Type with symbol name*/
            } else if (format[i+1] == 'n') {
                type* DT = va_arg(args, type*);
                char* ident = va_arg(args, char*);
                char* identStr = malloc(strlen(colourIdent)+strlen(ident)+strlen(colourType)+1);
                sprintf(identStr, "%s%s%s", colourIdent, ident, colourType);
                char* typeStr = typeToStr(DT, identStr);
                printf("%s%s%s", colourType, typeStr, consoleNormal);
                free(identStr);
                free(typeStr);

            } else if (format[i+1] == '\0')
                break;

            /*Unknown, ignore*/
            else
                {}

            i++;

        } else
            putchar(format[i]);
    }
}

static void errorParser (parserCtx* ctx, const char* format, ...) {
    tokenLocationMsg(ctx->location);
    printf("%serror%s: ", consoleRed, consoleNormal);

    va_list args;
    va_start(args, format);
    errorHighlighted(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;
    debugWait();
}

static void errorAnalyzer (analyzerCtx* ctx, const ast* Node, const char* format, ...) {
    tokenLocationMsg(Node->location);
    printf("%serror%s: ", consoleRed, consoleNormal);

    va_list args;
    va_start(args, format);
    errorHighlighted(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;
    debugWait();
}

/*:::: PARSER ERRORS ::::*/

void errorExpected (parserCtx* ctx, const char* expected) {
    errorParser(ctx, "expected $h, found '$h'", expected, ctx->lexer->buffer);
}

void errorUndefSym (parserCtx* ctx) {
    errorParser(ctx, "'$h' undefined", ctx->lexer->buffer);
}

void errorUndefType (parserCtx* ctx) {
    errorParser(ctx, "'$h' undefined, expected type", ctx->lexer->buffer);
}

void errorIllegalOutside (parserCtx* ctx, const char* what, const char* where) {
    errorParser(ctx, "illegal $s outside of $s", what, where);
}

void errorRedeclaredSymAs (parserCtx* ctx, const sym* Symbol, symTag tag) {
    const ast* first = (const ast*) vectorGet(&Symbol->decls, 0);

    errorParser(ctx, "'$h' redeclared as $h", Symbol->ident, symTagGetStr(tag));

    tokenLocationMsg(first->location);
    printf("first declaration here as $h\n", symTagGetStr(Symbol->tag));
}

void errorReimplementedSym (parserCtx* ctx, const sym* Symbol) {
    errorParser(ctx, "$h '$h' reimplemented",
          Symbol->tag == symId ? "function" : symTagGetStr(Symbol->tag), Symbol->ident);

    tokenLocationMsg(Symbol->impl->location);
    puts("first implementation here");
}

void errorFileNotFound (parserCtx* ctx, const char* name) {
    errorParser(ctx, "file not found, '$h'", name);
}

/*:::: ANALYZER ERRORS ::::*/

void errorTypeExpected (analyzerCtx* ctx, const ast* Node, const char* where, const char* expected) {
    errorAnalyzer(ctx, Node, "$s requires $s, found $t", where, expected, Node->dt);
}

void errorTypeExpectedType (analyzerCtx* ctx, const ast* Node, const char* where, const type* expected) {
    errorAnalyzer(ctx, Node, "$s requires $t, found $t", where, expected, Node->dt);
}

void errorLvalue (analyzerCtx* ctx, const ast* Node, const char* o) {
    errorAnalyzer(ctx, Node, "$o requires lvalue", o);
}

void errorMismatch (analyzerCtx* ctx, const ast* Node, const char* o) {
    errorAnalyzer(ctx, Node, "type mismatch between $t and $t for $o",
                  Node->l->dt, Node->r->dt, o);
}

void errorDegree (analyzerCtx* ctx, const ast* Node,
                  const char* thing, int expected, int found, const char* where) {
    errorAnalyzer(ctx, Node, "$h expected $d $s, $d given", where, expected, thing, found);
}

void errorParamMismatch (analyzerCtx* ctx, const ast* Node,
                         int n, const type* expected, const type* found) {
    errorAnalyzer(ctx, Node, "type mismatch at parameter $d, $t: found $t",
                  n+1, expected, found);
}

void errorNamedParamMismatch (analyzerCtx* ctx, const ast* Node,
                              int n, const sym* fn, const type* found) {
    const sym* param = symGetChild(fn, n);
    errorAnalyzer(ctx, Node, "type mismatch at parameter $d of $h, $n: found $t",
                  n+1, fn->ident, param->dt, param->ident ? param->ident : "", found);
}

void errorMember (analyzerCtx* ctx, const char* o, const ast* Node, const type* record) {
    errorAnalyzer(ctx, Node, "$o expected field of $t, found $h", o, record, Node->literal);
}

void errorInitFieldMismatch (analyzerCtx* ctx, const ast* Node,
                             const sym* structSym, const sym* fieldSym) {
    errorAnalyzer(ctx, Node, "type mismatch: $n given for initialization of field $n in $h $h",
                  Node->dt, (Node->symbol && Node->symbol->ident) ? Node->symbol->ident : "",
                  fieldSym->dt, fieldSym->ident,
                  symTagGetStr(structSym->tag), structSym->ident);
}

void errorConflictingDeclarations (analyzerCtx* ctx, const ast* Node,
                                   const sym* Symbol, const type* found) {
    errorAnalyzer(ctx, Node, "$n redeclared as conflicting type $t", Symbol->dt, Symbol->ident, found);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (Current->location.line != Node->location.line) {
            tokenLocationMsg(Current->location);
            printf("also declared here\n");
        }
    }
}

void errorRedeclared (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    if (Symbol->tag == symId || Symbol->tag == symParam)
        errorAnalyzer(ctx, Node, "$n redeclared", Symbol->dt, Symbol->ident);

    else
        errorAnalyzer(ctx, Node, "$h $h redeclared", symTagGetStr(Symbol->tag), Symbol->ident);

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
    errorAnalyzer(ctx, Node, "cannot use $h $n as a value", symTagGetStr(Symbol->tag), Symbol->dt, Symbol->ident);
}

void errorCompileTimeKnown (analyzerCtx* ctx, const ast* Node, const sym* Symbol, const char* what) {
    errorAnalyzer(ctx, Node, "declaration of $h needed a compile-time known $s", Symbol->ident, what);
}

void errorCompoundLiteralWithoutType (struct analyzerCtx* ctx, const struct ast* Node) {
    errorAnalyzer(ctx, Node, "compound literal without explicit type");
}
