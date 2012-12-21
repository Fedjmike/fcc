#include "../inc/debug.h"
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

void debugEnter (char* str) {
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

void debugMsg (char* format, ...) {
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

void report (char* str) {
    fputs(str, logFile);
}

void reportType (type DT) {
    char* Str = typeToStr(DT);
    fprintf(logFile, "type: %s\n", Str);
    free(Str);
}

void reportSymbol (sym* Symbol) {
    /*Class*/

    fprintf(logFile, "class: %s   ",
            symClassGetStr(Symbol->class));

    /*Symbol name*/

    fprintf(logFile, "symbol: %s   ",
            Symbol->ident);

    /*Parent*/

    if (Symbol->class != symGlobal &&
        Symbol->class != symType &&
        Symbol->class != symStruct &&
        Symbol->class != symFunction)
        fprintf(logFile, "parent: %s   ",
                Symbol->parent ? Symbol->parent->ident : "undefined");

    /*Type*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam ||
        Symbol->class == symFunction) {
        char* Str = typeToStr(Symbol->dt);
        fprintf(logFile, "type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam) {
        if (Symbol->dt.array == 0)
            fprintf(logFile, "size:   %d   ",
                    Symbol->dt.basic ? typeGetSize(Symbol->dt) : 0);

        else
            fprintf(logFile, "size: %dx%d   ",
                    Symbol->dt.basic ? Symbol->dt.basic->size : 0,
                    Symbol->dt.array);

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

void reportNode (ast* Node) {
    fprintf(logFile, "node: %p   class: %s   next: %p   fc: %p   l: %p\n",
            (void*) Node,
            astClassGetStr(Node->class),
            (void*) Node->nextSibling,
            (void*) Node->firstChild,
            (void*) Node->l);
}

void reportRegs () {
    fprintf(logFile, "[ ");

    for (int i = 0; i < regMax; i++)
        if (regIsUsed(i))
            fprintf(logFile, "%s ", regToStr(i));

    fprintf(logFile, "]\n");
}
