#include "../inc/debug.h"

#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/reg.h"

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

void debugWait () {
    if (mode <= debugMinimal)
        getchar();
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
    va_list args;
    va_start(args, format);
    debugVarMsg(format, args);
    va_end(args);
}

void debugVarMsg (const char* format, va_list args) {
    if (mode == debugSilent)
        return;

    for (int i = 0; i < depth; i++) {
        fputc('|', logFile);
        fputc(' ', logFile);
    }

    vfprintf(logFile, format, args);

    fputc('\n', logFile);
}

/*:::: INTERNAL ERRORS ::::*/

void debugError (const char* functionName,
                 const char* format, ...) {
    debugMsg("internal error(%s): ", functionName);

    va_list args;
    va_start(args, format);
    debugVarMsg(format, args);
    va_end(args);

    debugWait();
}

void debugAssert (const char* functionName,
                  const char* testName,
                  bool result) {
    if (!result)
        debugError(functionName, "%s assertion failed", testName);
}

void debugErrorUnhandled (const char* functionName,
                          const char* className,
                          const char* classStr) {
    debugError(functionName, "unhandled %s: '%s'", className, classStr);
}

void debugErrorUnhandledInt (const char* functionName,
                             const char* className,
                             int classInt) {
    debugError(functionName, "unhandled %s: %d", className, classInt);
}

/*:::: REPORTING INTERNAL STRUCTURES ::::*/

void report (const char* str) {
    fputs(str, logFile);
}

void reportType (const type* DT) {
    fprintf(logFile, "type: %p   ",
            (void*) DT);

    if (DT != 0) {
        /*Tag*/

        fprintf(logFile, "tag: %s   ",
                typeTagGetStr(DT->tag));

        /*Base type*/

        if (typeIsPtr(DT) || typeIsArray(DT))
            fprintf(logFile, "base: %p   ",
                    (void*) DT->base);

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
    /*Tag*/

    fprintf(logFile, "tag: %s   ",
            symTagGetStr(Symbol->tag));

    /*Symbol name*/

    if (Symbol->tag != symScope)
        fprintf(logFile, "symbol: %s   ",
                Symbol->ident);

    /*Parent*/

    if (   Symbol->tag != symType
        && Symbol->tag != symStruct)
        fprintf(logFile, "parent: %s   ",
                Symbol->parent ? Symbol->parent->ident : "undefined");

    /*Type*/

    if (   (   Symbol->tag == symId
            || Symbol->tag == symParam)
        && Symbol->dt != 0) {
        char* Str = typeToStr(Symbol->dt, "");
        fprintf(logFile, "type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    /*if (   (   Symbol->tag == symId
            || Symbol->tag == symParam)
        && Symbol->dt != 0) {
        if (Symbol->dt->tag == typeArray)
            fprintf(logFile, "size: %dx%d   ",
                    typeGetSize(Symbol->dt->base),
                    Symbol->dt->array);

        else
            fprintf(logFile, "size:   %d   ",
                    typeGetSize(Symbol->dt));

    } else if (Symbol->tag == symStruct)
        fprintf(logFile, "size:   %d   ",
                Symbol->size);*/

    /*Offset*/

    if (   Symbol->tag == symId
        || Symbol->tag == symParam)
        fprintf(logFile, "offset: %d",
                Symbol->offset);

    fputc('\n', logFile);
}

void reportNode (const ast* Node) {
    fprintf(logFile, "node: %p   tag: %s   ",
            (void*) Node,
            astTagGetStr(Node->tag));

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

    for (regIndex r = 0; r < regMax; r++)
        if (regIsUsed(r))
            fprintf(logFile, "%s ", regToStr(&regs[r]));

    fprintf(logFile, "]\n");
}
