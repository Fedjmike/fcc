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
    const char* consoleNormal  = "\e[1;0m";
    const char* consoleRed     = "\e[1;31m";
    const char* consoleGreen   = "\e[1;32m";
    //const char* consoleYellow  = "\e[1;33m";
    //const char* consoleBlue    = "\e[1;34m";
    const char* consoleMagenta = "\e[1;35m";
    const char* consoleCyan    = "\e[1;36m";
    const char* consoleWhite   = "\e[1;37m";

    const char* colourString = consoleWhite;
    const char* colourNumber = consoleMagenta;
    const char* colourOp = consoleCyan;
    const char* colourType = consoleGreen;
    const char* colourIdent = consoleWhite;
    const char* colourTag = consoleCyan;

    int flen = strlen(format);

    for (int i = 0; i < flen; i++) {
        if (format[i] == '$') {
            i++;

            /*Regular string*/
            if (format[i] == 's')
                printf("%s", va_arg(args, char*));

            /*Highlighted string*/
            else if (format[i] == 'h')
                printf("%s%s%s", colourString, va_arg(args, char*), consoleNormal);

            /*Red string*/
            else if (format[i] == 'r')
                printf("%s%s%s", consoleRed, va_arg(args, char*), consoleNormal);

            /*Operator*/
            else if (format[i] == 'o')
                printf("%s%s%s", colourOp, opTagGetStr(va_arg(args, opTag)), consoleNormal);

            /*Integer*/
            else if (format[i] == 'd')
                printf("%s%d%s", colourNumber, va_arg(args, int), consoleNormal);

            /*Raw type*/
            else if (format[i] == 't') {
                char* typeStr = typeToStr(va_arg(args, type*));
                printf("%s%s%s", colourType, typeStr, consoleNormal);
                free(typeStr);

            /*Named symbol*/
            } else if (format[i] == 'n') {
                const sym* Symbol = va_arg(args, sym*);
                const char* ident = Symbol->ident ? Symbol->ident : "";

                if (Symbol->tag == symId || Symbol->tag == symParam) {
                    if (Symbol->dt) {
                        char* identStr = strjoin((char**) (const char* []) {colourIdent, ident, colourType}, 3, malloc);
                        char* typeStr = typeToStrEmbed(Symbol->dt, identStr);
                        printf("%s%s%s", colourType, typeStr, consoleNormal);
                        free(identStr);
                        free(typeStr);

                    } else
                        printf("%s%s%s", colourIdent, ident, consoleNormal);

                } else
                    printf("%s%s%s %s%s", colourTag, symTagGetStr(Symbol->tag), colourIdent, ident, consoleNormal);

            /*AST node*/
            } else if (format[i] == 'a') {
                const ast* Node = va_arg(args, ast*);

                if (Node->symbol && Node->symbol->ident && Node->symbol->dt)
                    if (Node->symbol->tag == symId || Node->symbol->tag == symParam) {
                        char* identStr = strjoin((char**) (const char* []) {colourIdent, Node->symbol->ident, colourType}, 3, malloc);
                        char* typeStr = typeToStrEmbed(Node->dt, identStr);
                        printf("%s%s%s", colourType, typeStr, consoleNormal);
                        free(identStr);
                        free(typeStr);

                    } else
                        errorf("$n", Node->symbol);

                else
                    errorf("$t", Node->dt);

            /*Symbol tag, semantic "class"*/
            } else if (format[i] == 'c') {
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

            } else if (format[i] == '\0')
                break;

            else
                debugErrorUnhandledChar("verrorf", "format specifier", format[i]);

        } else
            putchar(format[i]);
    }
}

static void errorParser (parserCtx* ctx, const char* format, ...) {
    if (ctx->location.line == ctx->lastErrorLine)
        return;

    tokenLocationMsg(ctx->location);
    errorf("$r: ", "error");

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
    errorf("$r: ", "error");

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

    errorParser(ctx, "$h redeclared as $s", Symbol->ident, tag != symId ? symTagGetStr(tag) : "different symbol type");

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
    errorAnalyzer(ctx, Node, "$h requires $s, found $a", where, expected, Node);
}

void errorOpTypeExpected (analyzerCtx* ctx, const ast* Node, opTag o, const char* expected) {
    errorAnalyzer(ctx, Node, "$o requires $s, found $a", o, expected, Node);
}

void errorTypeExpectedType (analyzerCtx* ctx, const ast* Node, const char* where, const type* expected) {
    errorAnalyzer(ctx, Node, "$s requires $t, found $t", where, expected, Node->dt);
}

void errorLvalue (analyzerCtx* ctx, const ast* Node, opTag o) {
    errorAnalyzer(ctx, Node, "$o requires an lvalue", o);
}

void errorMismatch (analyzerCtx* ctx, const ast* Node, opTag o) {
    errorAnalyzer(ctx, Node, "type mismatch between $t and $t for $o",
                  Node->l->dt, Node->r->dt, o);
}

void errorVoidDeref (analyzerCtx* ctx, const ast* Node, opTag o) {
    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "$o dereference of $h would result in void value", o, Node->symbol->ident);

    else
        errorAnalyzer(ctx, Node, "$o derefence would result in void value", o);
}

void errorDegree (analyzerCtx* ctx, const ast* Node,
                  const char* thing, int expected, int found, const char* where) {
    errorAnalyzer(ctx, Node, "too $s $s given to $s: expected $d, given $d",
                  expected > found ? "few" : "many", thing, where, expected, found);
}

void errorParamMismatch (analyzerCtx* ctx, const ast* Node,
                         const ast* fn, int n, const type* expected, const type* found) {
    if (fn->symbol) {
        const sym* param = symGetNthParam(fn->symbol, n);

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

void errorMember (analyzerCtx* ctx, const ast* Node, opTag o, const char* field) {
    errorAnalyzer(ctx, Node, "$o expected field of $a, found $h", o, Node->l, field);
}

void errorVAxList (analyzerCtx* ctx, const ast* Node, const char* where, const char* which) {
    errorAnalyzer(ctx, Node, "$s parameter of $h requires $h", which, where, "va_list");
}

void errorVAxLvalue (analyzerCtx* ctx, const ast* Node, const char* where, const char* which) {
    errorAnalyzer(ctx, Node, "$s parameter of $h requires an lvalue", which, where);
}

void errorVAStartNonParam (analyzerCtx* ctx, const ast* Node) {
    errorAnalyzer(ctx, Node, "$h expected parameter name, found $n", "va_start", Node->symbol);
}

void errorIllegalInit (analyzerCtx* ctx, const ast* Node, const char* what) {
    errorAnalyzer(ctx, Node, "illegal initialization of $s $h",
                  what, Node->l->symbol->ident ? Node->l->symbol->ident : "\b");
}

void errorInitMismatch (analyzerCtx* ctx, const ast* variable, const ast* init) {
    errorAnalyzer(ctx, init, "incompatible initialization of $n from $a", variable->symbol, init);
}

void errorInitFieldMismatch (analyzerCtx* ctx, const ast* Node,
                             const sym* record, const sym* field) {
    errorAnalyzer(ctx, Node, "type mismatch: initialization of $n field $n given $t",
                  record, field, Node->dt);
}

void errorInitExcessFields (analyzerCtx* ctx, const ast* Node, const sym* record, const sym* field) {
    if (field)
        errorAnalyzer(ctx, Node, "excess initializers after the last field of $n, $n", record, field);

    else
        errorAnalyzer(ctx, Node, "excess initializers after the last field of $n", record);
}

void errorWrongInitDesignator (analyzerCtx* ctx, const ast* Node, const type* DT) {
    const char *got, *expected;

    if (typeIsArray(DT)) {
        got = "struct";
        expected = "array";

    } else {
        got = "array";
        expected = "struct";
    }

    errorAnalyzer(ctx, Node, "$s designator given for initialization of $s $t",
                  got, expected, DT);
}

void errorConstantInitIndex (analyzerCtx* ctx, ast* Node) {
    errorAnalyzer(ctx, Node, "non-constant index given to designated initializer");
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
        errorAnalyzer(ctx, Node, "$h was already qualified with $h", Node->symbol->ident, "const");

    else
        errorAnalyzer(ctx, Node, "type was already qualified with $h", "const");
}

void errorIllegalConst (analyzerCtx* ctx, const ast* Node, const type* DT) {
    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "illegal qualification of a$s, $n, as $h",
                      typeIsArray(DT) ? "n array" : " function", Node->symbol, "const");

    else
        errorAnalyzer(ctx, Node, "illegal qualification of a$s as $h",
                      typeIsArray(DT) ? "n array" : " function", "const");
}

void errorIllegalSymAsValue (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    errorAnalyzer(ctx, Node, "cannot use $n as a value", Symbol);
}

void errorCompileTimeKnown (analyzerCtx* ctx, const ast* Node, const sym* Symbol, const char* what) {
    errorAnalyzer(ctx, Node, "declaration of $h needed a compile-time known $s", Symbol->ident, what);
}

void errorStaticCompileTimeKnown (analyzerCtx* ctx, const ast* Node, const sym* Symbol) {
    errorAnalyzer(ctx, Node, "initialization of static variable $h needed a compile-time known value", Symbol->ident);
}

void errorIllegalArraySize (analyzerCtx* ctx, const ast* Node,
                            const sym* Symbol, int size) {
    errorAnalyzer(ctx, Node, "declaration of array $s expected positive size, found $d", Symbol, size);
}

void errorCompoundLiteralWithoutType (analyzerCtx* ctx, const ast* Node) {
    errorAnalyzer(ctx, Node, "compound literal is missing an explicit type");
}

void errorIncompletePtr (analyzerCtx* ctx, const ast* Node, opTag o) {
    const sym* basic = typeGetBasic(typeGetBase(Node->dt));

    /*Only error once per incomplete type*/
    if (!(basic && intsetAdd(&ctx->incompletePtrIgnore, (intptr_t) basic)))
        errorAnalyzer(ctx, Node, "$o cannot dereference incomplete pointer $a", o, Node);
}


void errorIncompleteCompound (analyzerCtx* ctx, const ast* Node, const type* DT) {
    errorAnalyzer(ctx, Node, "cannot initialize compound literal of incomplete type $t", DT);
}

void errorIncompleteDecl (analyzerCtx* ctx, const ast* Node, const type* DT) {
    const sym* basic = typeGetBasic(Node->dt);

    /*Again, only once (decl and ptr errors counted separately)*/
    if (basic && intsetAdd(&ctx->incompleteDeclIgnore, (intptr_t) basic))
        return;

    if (Node->symbol && Node->symbol->ident)
        errorAnalyzer(ctx, Node, "$n declared with incomplete type", Node->symbol);

    else
        errorAnalyzer(ctx, Node, "variable declared with incomplete type $t", DT);
}

void errorIncompleteParamDecl (analyzerCtx* ctx, const ast* Node, const ast* fn, int n, const type* DT) {
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
        errorAnalyzer(ctx, Node, "parameter $d$s$h declared with incomplete type $t", n, of, ident, DT);
}

void errorIncompleteReturnDecl (analyzerCtx* ctx, const ast* Node, const type* dt) {
    errorAnalyzer(ctx, Node, "function $n declared with incomplete return type $t", Node->symbol, dt);
}

void errorConstAssignment (analyzerCtx* ctx, const ast* Node, opTag o) {
    errorAnalyzer(ctx, Node, "$o tried to modify immutable $a", o, Node);
}

void errorFnTag (analyzerCtx* ctx, ast* Node) {
    errorAnalyzer(ctx, Node, "function implementation illegally given to $n, a $c",
                  Node->symbol, Node->symbol);
}

void errorReturnType (analyzerCtx* ctx, ast* Node, analyzerFnCtx fnctx) {
    if (fnctx.fn && fnctx.fn->ident)
        errorAnalyzer(ctx, Node, "return type of $h declared as $t, given $t",
                      fnctx.fn->ident, fnctx.returnType, Node->dt);

    else
        errorAnalyzer(ctx, Node, "return statement requires $t, given $t",
                      fnctx.returnType, Node->dt);
}
