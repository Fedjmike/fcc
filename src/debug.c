#include "../inc/debug.h"

#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/reg.h"
#include "../inc/operand.h"

#include "../inc/architecture.h"

#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"

FILE* logFile;
debugMode mode;
//Indentation level of debug output
int depth;

int internalErrors;

void debugInit (FILE* nlog) {
    logFile = nlog;
    debugSetMode(debugMinimal);
}

debugMode debugSetMode (debugMode nmode) {
    debugMode old = mode;
    mode = nmode;
    return old;
}

void debugWait () {
    #ifdef FCC_DEBUGMODE
    if (mode <= debugFull)
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
    for (int i = 0; i < depth; i++)
        debugOut("| ");

    debugVarOut(format, args);
    debugOut("\n");
}

void debugOut (const char* format, ...) {
    va_list args;
    va_start(args, format);
    debugVarOut(format, args);
    va_end(args);
}

void debugVarOut (const char* format, va_list args) {
    (void) args, (void) format;

    #ifdef FCC_DEBUGMODE

    if (mode == debugSilent)
        return;

    vfprintf(logFile, format, args);

    #endif
}

/*==== Internal errors ====*/

void debugError (const char* functionName,
                 const char* format, ...) {
    fprintf(logFile, "internal error(%s): ", functionName);

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fputc('\n', logFile);

    debugWait();
    internalErrors++;
}

bool debugAssert (const char* functionName,
                  const char* testName,
                  bool result) {
    if (!result)
        debugError(functionName, "%s assertion failed", testName);

    return !result;
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

void debugErrorUnhandledChar (const char* functionName,
                              const char* className,
                              char classChar) {
    debugError(functionName, "unhandled %s: '%c'", className, classChar);
}

/*==== Reporting internal structures ====*/

void report (const char* str) {
    debugOut("%s\n", str);
}

void reportType (const type* DT) {
    debugOut("type: %p   ", (void*) DT);

    if (DT != 0) {
        /*Tag*/

        debugOut("tag: %s   ", typeTagGetStr(DT->tag));

        /*Base type*/

        if (typeIsPtr(DT) || typeIsArray(DT))
            debugOut("base: %p   ", (void*) typeGetBase(DT));

        /*String form*/

        if (!typeIsInvalid(DT)) {
            char* Str = typeToStr(DT);
            debugOut("str: %s   ", Str);
            free(Str);
        }
    }

    debugOut("\n");
}

void reportSymbol (const sym* Symbol) {
    /*Tag*/

    debugOut("tag: %s   ", symTagGetStr(Symbol->tag));

    /*Symbol name*/

    if (Symbol->tag != symScope)
        debugOut("symbol: %s   ", Symbol->ident);

    /*Parent*/

    if (   Symbol->tag != symType
        && Symbol->tag != symStruct)
        debugOut("parent: %s   ",
                 Symbol->parent ? Symbol->parent->ident : "undefined");

    /*Type*/

    if (   (   Symbol->tag == symId
            || Symbol->tag == symParam)
        && Symbol->dt != 0) {
        char* Str = typeToStr(Symbol->dt);
        debugOut("type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    /*if (   (   Symbol->tag == symId
            || Symbol->tag == symParam)
        && Symbol->dt != 0) {
        if (typeIsArray(Symbol->dt))
            debugOut("size: %dx%d   ",
                     typeGetSize(typeGetBase(Symbol->dt)),
                     typeGetArraySize(Symbol->dt));

        else
            debugOut("size:   %d   ", typeGetSize(Symbol->dt));

    } else if (Symbol->tag == symStruct)
        debugOut("size:   %d   ", Symbol->size);*/

    /*Offset*/

    if (   Symbol->tag == symId
        || Symbol->tag == symParam)
        debugOut("offset: %d", Symbol->offset);

    debugOut("\n");
}

void reportSymbolTree (const sym* Symbol, int level) {
    for (int i = 0; i < level; i++)
        debugOut("| ");

    debugOut("+ %s", symTagGetStr(Symbol->tag));

    if (Symbol->ident)
        debugOut(" %s", Symbol->ident);

    debugOut("\n");

    if (Symbol->tag == symModuleLink)
        return;

    for (int i = 0; i < Symbol->children.length; i++) {
        sym* current = vectorGet(&Symbol->children, i);
        reportSymbolTree(current, level+1);
    }
}

void reportNode (const ast* Node) {
    debugOut("node: %p   tag: %s   ",
             (void*) Node,
             astTagGetStr(Node->tag));

    if (Node->nextSibling)
        debugOut("next: %p   ", (void*) Node->nextSibling);

    if (Node->firstChild)
        debugOut("fc: %p   ", (void*) Node->firstChild);

    if (Node->l)
        debugOut("l: %p", (void*) Node->l);

    debugOut("\n");
}

void reportRegs () {
    debugOut("[ ");

    for (regIndex r = 0; r < regMax; r++)
        if (regIsUsed(r))
            debugOut("%s ", regGetStr(regGet(r)));

    debugOut(" ]\n");
}

void reportOperand (const architecture* arch, const operand* R) {
    char* RStr = operandToStr(*R);

    debugOut("tag: %s   size: %d   str: %s\n",
             operandTagGetStr(R->tag),
             operandGetSize(arch, *R),
             RStr);

    free(RStr);
}
