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
    #ifdef FCC_DEBUGMODE
    if (mode <= debugMinimal)
        getchar();
    #endif
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
    (void) args;
    (void) format;

    #ifdef FCC_DEBUGMODE

    if (mode == debugSilent)
        return;

    for (int i = 0; i < depth; i++) {
        fputc('|', logFile);
        fputc(' ', logFile);
    }

    vfprintf(logFile, format, args);

    fputc('\n', logFile);

    #endif
}

/*:::: INTERNAL ERRORS ::::*/

void debugError (const char* functionName,
                 const char* format, ...) {
    fprintf(logFile, "internal error(%s): ", functionName);

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fputc('\n', logFile);

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
    debugMsg("%s\n", str);
}

void reportType (const type* DT) {
    debugMsg("type: %p   ", (void*) DT);

    if (DT != 0) {
        /*Tag*/

        debugMsg("tag: %s   ", typeTagGetStr(DT->tag));

        /*Base type*/

        if (typeIsPtr(DT) || typeIsArray(DT))
            debugMsg("base: %p   ", (void*) DT->base);

        /*String form*/

        if (!typeIsInvalid(DT)) {
            char* Str = typeToStr(DT, "");
            debugMsg("str: %s   ", Str);
            free(Str);
        }
    }

    debugMsg("\n");
}

void reportSymbol (const sym* Symbol) {
    /*Tag*/

    debugMsg("tag: %s   ", symTagGetStr(Symbol->tag));

    /*Symbol name*/

    if (Symbol->tag != symScope)
        debugMsg("symbol: %s   ", Symbol->ident);

    /*Parent*/

    if (   Symbol->tag != symType
        && Symbol->tag != symStruct)
        debugMsg("parent: %s   ",
                 Symbol->parent ? Symbol->parent->ident : "undefined");

    /*Type*/

    if (   (   Symbol->tag == symId
            || Symbol->tag == symParam)
        && Symbol->dt != 0) {
        char* Str = typeToStr(Symbol->dt, "");
        debugMsg("type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    /*if (   (   Symbol->tag == symId
            || Symbol->tag == symParam)
        && Symbol->dt != 0) {
        if (Symbol->dt->tag == typeArray)
            debugMsg("size: %dx%d   ",
                     typeGetSize(Symbol->dt->base),
                     Symbol->dt->array);

        else
            debugMsg("size:   %d   ", typeGetSize(Symbol->dt));

    } else if (Symbol->tag == symStruct)
        debugMsg("size:   %d   ", Symbol->size);*/

    /*Offset*/

    if (   Symbol->tag == symId
        || Symbol->tag == symParam)
        debugMsg("offset: %d", Symbol->offset);

    debugMsg("\n");
}

void reportNode (const ast* Node) {
    debugMsg("node: %p   tag: %s   ",
             (void*) Node,
             astTagGetStr(Node->tag));

    if (Node->nextSibling)
        debugMsg("next: %p   ", (void*) Node->nextSibling);

    if (Node->firstChild)
        debugMsg("fc: %p   ", (void*) Node->firstChild);

    if (Node->l)
        debugMsg("l: %p", (void*) Node->l);

    debugMsg("\n");
}

void reportRegs () {
    debugMsg("[ ");

    for (regIndex r = 0; r < regMax; r++)
        if (regIsUsed(r))
            debugMsg("%s ", regToStr(&regs[r]));

    debugMsg(" ]\n");
}
