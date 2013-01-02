#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"

#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

FILE* logFile;
debugMode mode;
//Indentation level of debug output
int depth;

void debugInit (FILE* nlog) {
    logFile = nlog;
    debugSetMode(debugFull);
}

debugMode debugSetMode (debugMode nmode) {
    debugMode old = mode;
    mode = nmode;
    return old;
}

void debugEnter (const char* str) {
    if (mode <= debugCompressed) {
        debugMsg("+ %s", str);
        depth++;
    }
}

void debugLeave () {
    if (mode <= debugCompressed)
        depth--;

    if (mode == debugFull)
        debugMsg("-");
}

void debugMsg (const char* format, ...) {
    if (mode == debugSilent)
        return;

    for (int i = 0; i < depth; i++) {
        fputc('|', logFile);
        fputc(' ', logFile);
    }

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fputc('\n', logFile);
}

/*:::: INTERNAL ERRORS ::::*/

void debugAssert (const char* functionName,
                  const char* testName,
                  bool result) {
    if (!result)
        debugMsg("internal error(%s): %s assertion failed",
                 functionName, testName);
}

void debugErrorUnhandled (const char* functionName,
                          const char* className,
                          const char* classStr) {
    debugMsg("internal error(%s): unhandled %s: '%s'",
             functionName, className, classStr);
}

/*:::: REPORTING INTERNAL STRUCTURES ::::*/

void report (const char* str) {
    fputs(str, logFile);
}

void reportType (const type* DT) {
    fprintf(logFile, "type: %p   ",
            (void*) DT);

    if (DT != 0) {
        /*Class*/

        fprintf(logFile, "class: %s   ",
                typeClassGetStr(DT->class));

        /*Base type*/

        if (typeIsPtr(DT) || typeIsArray(DT))
            fprintf(logFile, "base: %p   ",
                    DT->base);

        /*String form*/

        if (!typeIsInvalid(DT)) {
            char* Str = typeToStr(DT, "");
            fprintf(logFile, "str: %s   ", Str);
            free(Str);
        }
    }

    fputc('\n', logFile);
}

void reportSymbol (const sym* Symbol) {
    /*Class*/

    fprintf(logFile, "class: %s   ",
            symClassGetStr(Symbol->class));

    /*Symbol name*/

    if (Symbol->class != symScope)
        fprintf(logFile, "symbol: %s   ",
                Symbol->ident);

    /*Parent*/

    if (Symbol->class != symType &&
        Symbol->class != symStruct &&
        Symbol->class != symFunction)
        fprintf(logFile, "parent: %s   ",
                Symbol->parent ? Symbol->parent->ident : "undefined");

    /*Type*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam ||
        Symbol->class == symFunction) {
        char* Str = typeToStr(Symbol->dt, "");
        fprintf(logFile, "type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    if ((Symbol->class == symVar ||
        Symbol->class == symParam) &&
        Symbol->dt != 0) {
        if (typeIsArray(Symbol->dt))
            fprintf(logFile, "size: %dx%d   ",
                    typeGetSize(Symbol->dt->base),
                    Symbol->dt->array);

        else
            fprintf(logFile, "size:   %d   ",
                    typeGetSize(Symbol->dt));

    } else if (Symbol->class == symStruct)
        fprintf(logFile, "size:   %d   ",
                Symbol->size);

    /*Offset*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam)
        fprintf(logFile, "offset: %d",
                Symbol->offset);

    fputc('\n', logFile);
}

void reportNode (const ast* Node) {
    fprintf(logFile, "node: %p   class: %s   ",
            (void*) Node,
            astClassGetStr(Node->class));

    if (Node->nextSibling)
        fprintf(logFile, "next: %p   ",
                (void*) Node->nextSibling);

    if (Node->firstChild)
        fprintf(logFile, "fc: %p   ",
                (void*) Node->firstChild);

    if (Node->l)
        fprintf(logFile, "l: %p",
                (void*) Node->l);

    fputc('\n', logFile);
}

void reportRegs () {
    fprintf(logFile, "[ ");

    for (int i = 0; i < regMax; i++)
        if (regIsUsed(i))
            fprintf(logFile, "%s ", regToStr(i));

    fprintf(logFile, "]\n");
}
