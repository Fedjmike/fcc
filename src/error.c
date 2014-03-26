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

static void tokenLocationMsg (tokenLocation loc);
static void errorf (const char* format, ...);
static void verrorf (const char* format, va_list args);
static void errorParser (parserCtx* ctx, const char* format, ...);
static void errorAnalyzer (analyzerCtx* ctx, const ast* Node, const char* format, ...);

static void tokenLocationMsg (tokenLocation loc) {
    printf("%s:%d:%d: ", loc.filename, loc.line, loc.lineChar);
}

static void errorf (const char* format, ...) {
    va_list args;
    va_start(args, format);
    verrorf(format, args);
    va_end(args);
}

static void verrorf (const char* format, va_list args) {
    const char* colourString = consoleWhite;
    const char* colourNumber = consoleMagenta;
    const char* colourOp = consoleCyan;
    const char* colourType = consoleGreen;
    const char* colourIdent = consoleWhite;
    const char* colourTag = consoleCyan;

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
                char* typeStr = typeToStr(va_arg(args, type*));
                printf("%s%s%s", colourType, typeStr, consoleNormal);
                free(typeStr);

            /*Named symbol*/
            } else if (format[i+1] == 'n') {
                const sym* Symbol = va_arg(args, sym*);
                const char* ident = Symbol->ident ? Symbol->ident : "";

                if ((Symbol->tag == symId || Symbol->tag == symParam)) {
                    if (Symbol->dt) {
                        char* identStr = malloc(strlen(colourIdent)+strlen(ident)+strlen(colourType)+1);
                        sprintf(identStr, "%s%s%s", colourIdent, ident, colourType);
                        char* typeStr = typeToStrEmbed(Symbol->dt, identStr);
                        printf("%s%s%s", colourType, typeStr, consoleNormal);
                        free(identStr);
                        free(typeStr);

                    } else
                        printf("%s%s%s", colourIdent, ident, consoleNormal);

                } else
                    printf("%s%s%s %s%s", colourTag, symTagGetStr(Symbol->tag), colourIdent, ident, consoleNormal);

            /*AST node*/
            } else if (format[i+1] == 'a') {
                const ast* Node = va_arg(args, ast*);

                if (Node->symbol && Node->symbol->ident)
                    errorf("$n", Node->symbol);

                else
                    errorf("$t", Node->dt);

            /*Symbol tag, semantic "class"*/
            } else if (format[i+1] == 'c') {
                const sym* Symbol = va_arg(args, sym*);
                const char* classStr;

                if (Symbol->tag == symId || Symbol->tag == symParam) {
                    if (Symbol->dt && !typeIsInvalid(Symbol->dt))
                        classStr = typeIsFunction(Symbol->dt) ? "function" : "variable";

                    else
                        classStr = "\b";

                } else
                    classStr = symTagGetStr(Symbol->tag);

                printf("%s%s%s", colourTag, classStr, consoleNormal);

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
    if (ctx->location.line == ctx->lastErrorLine)
        return;

    tokenLocationMsg(ctx->location);
    printf("%serror%s: ", consoleRed, consoleNormal);

    va_list args;
    va_start(args, format);
    verrorf(format, args);
    va_end(args);

    putchar('\n');

    ctx->errors++;
    ctx->lastErrorLine = ctx->location.line;
    debugWait();
}

static void errorAnalyzer (analyzerCtx* ctx, const ast* Node, const char* format, ...) {
    tokenLocationMsg(Node->location);
    printf("%serror%s: ", consoleRed, consoleNormal);

    va_list args;
    va_start(args, format);
    verrorf(format, args);
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

    errorParser(ctx, "$h redeclared as $h", Symbol->ident, tag != symId ? symTagGetStr(tag) : "\b\b\b   ");

    tokenLocationMsg(first->location);
    errorf("first declaration here as $c\n", Symbol);
}

void errorReimplementedSym (parserCtx* ctx, const sym* Symbol) {
    errorParser(ctx, "$n reimplemented", Symbol);

    tokenLocationMsg(Symbol->impl->location);
    puts("first implementation here");
}

void errorFileNotFound (parserCtx* ctx, const char* name) {
    errorParser(ctx, "file not found, '$h'", name);
}

/*:::: ANALYZER ERRORS ::::*/

void errorTypeExpected (analyzerCtx* ctx, const ast* Node, const char* where, const char* expected) {
    errorAnalyzer(ctx, Node, "$o requires $s, found $a", where, expected, Node);
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
    errorAnalyzer(ctx, Node, "too $s $s given to $h: expected $d, given $d",
                  expected > found ? "few" : "many", thing, where, expected, found);
}

void errorParamMismatch (analyzerCtx* ctx, const ast* Node,
                         const ast* fn, int n, const type* expected, const type* found) {
    if (fn->symbol) {
        const sym* param = symGetChild(fn->symbol, n);

        if (param)
            errorAnalyzer(ctx, Node, "type mismatch at parameter $d of $h, $n: found $t",
                          n+1, fn->symbol->ident, param, found);

        else
            errorAnalyzer(ctx, Node, "type mismatch at parameter $d of $h, expected $t: found $t",
                          n+1, fn->symbol->ident, expected, found);

    } else
        errorAnalyzer(ctx, Node, "type mismatch at parameter $d, expected $t: found $t",
                  n+1, expected, found);
}

void errorMember (analyzerCtx* ctx, const ast* Node, const char* field) {
    errorAnalyzer(ctx, Node, "$o expected field of $a, found $h", Node->o, Node->l, field);
}

void errorInitMismatch (analyzerCtx* ctx, const ast* variable, const ast* init) {
    errorAnalyzer(ctx, init, "incompatible initialization of $a from $a", variable, init);
}

void errorInitFieldMismatch (analyzerCtx* ctx, const ast* Node,
                             const sym* structSym, const sym* fieldSym) {
    errorAnalyzer(ctx, Node, "type mismatch: $t given for initialization of field $n in $n",
                  Node->dt, fieldSym, structSym);
}

void errorConflictingDeclarations (analyzerCtx* ctx, const ast* Node,
                                   const sym* Symbol, const type* found) {
    errorAnalyzer(ctx, Node, "$n redeclared as conflicting type $t", Symbol, found);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (Current->location.line != Node->location.line) {
            tokenLocationMsg(Current->location);
            printf("also declared here\n");
        }
    }
}

void errorRedeclared (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    errorAnalyzer(ctx, Node, "$n redeclared", Symbol);

    for (int n = 0; n < Symbol->decls.length; n++) {
        const ast* Current = (const ast*) vectorGet(&Symbol->decls, n);

        if (   Current->location.line != Node->location.line
            || Current->location.filename != Node->location.filename) {
            tokenLocationMsg(Current->location);
            printf("also declared here\n");
        }
    }
}

void errorAlreadyConst (analyzerCtx* ctx, const ast* Node) {
    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "$h was already qualified with $h", Node->symbol->ident, "'const'");

    else
        errorAnalyzer(ctx, Node, "type was already qualified with $h", "'const'");
}

void errorIllegalConst (analyzerCtx* ctx, const ast* Node) {
    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "illegal qualification of a$s, $n, as $h",
                      typeIsArray(Node->dt) ? "n array" : " function", Node->symbol, "const");

    else
        errorAnalyzer(ctx, Node, "illegal qualification of a , $s, as $h",
                      typeIsArray(Node->dt) ? "array" : "function", "const");
}

void errorIllegalSymAsValue (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    errorAnalyzer(ctx, Node, "cannot use $n as a value", Symbol);
}

void errorCompileTimeKnown (analyzerCtx* ctx, const ast* Node, const sym* Symbol, const char* what) {
    errorAnalyzer(ctx, Node, "declaration of $h needed a compile-time known $s", Symbol->ident, what);
}

void errorCompoundLiteralWithoutType (analyzerCtx* ctx, const ast* Node) {
    errorAnalyzer(ctx, Node, "compound literal without explicit type");
}

void errorIncompletePtr (analyzerCtx* ctx, const ast* Node, const char* o) {
    const sym* basic = typeGetBasic(typeGetBase(Node->dt));

    /*Only error once per incomplete type*/
    if (!(basic && intsetAdd(&ctx->incompletePtrIgnore, (intptr_t) basic)))
        errorAnalyzer(ctx, Node, "$o cannot dereference incomplete pointer $a", o, Node);
}

void errorIncompleteDecl (analyzerCtx* ctx, const ast* Node) {
    const sym* basic = typeGetBasic(Node->dt);

    /*Again, only once (decl and ptr errors counted separately)*/
    if (basic && intsetAdd(&ctx->incompleteDeclIgnore, (intptr_t) basic))
        return;

    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "$n declared with incomplete type", Node->symbol);

    else
        errorAnalyzer(ctx, Node, "variable declared with incomplete type $t", Node->dt);
}

void errorIncompleteParamDecl (analyzerCtx* ctx, const ast* Node, const ast* fn, int n) {
    const sym* basic = typeGetBasic(Node->dt);

    if (basic && intsetAdd(&ctx->incompleteDeclIgnore, (intptr_t) basic))
        return;

    const char *of = "", *ident = "";

    if (fn->symbol && fn->symbol->ident) {
        of = " of function ";
        ident = fn->symbol->ident;
    }

    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "parameter $d$s$h, $h declared with incomplete type $t", n, of, ident, Node->symbol->ident, Node->dt);

    else
        errorAnalyzer(ctx, Node, "parameter $d$s$h declared with incomplete type $t", n, of, ident, Node->dt);
}

void errorIncompleteReturnDecl (analyzerCtx* ctx, const ast* Node, const type* dt) {
    errorAnalyzer(ctx, Node, "function $n declared with incomplete return type $t", Node->symbol, dt);
}

void errorConstAssignment (struct analyzerCtx* ctx, const struct ast* Node, const char* o) {
    errorAnalyzer(ctx, Node, "$o tried to modify immutable $a", o, Node);
}
