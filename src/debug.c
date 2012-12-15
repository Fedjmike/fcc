#include "stdlib.h"
#include "stdio.h"

#include "../inc/debug.h"

FILE* debugLog;
//Indentation level of debug output
int debugDepth;

void debugInit (FILE* log) {
    debugLog = log;
}

void debugEnter (char* str) {
    debugMsg("+ %s", str);
    debugDepth++;
}

void debugLeave () {
    debugDepth--;
    debugMsg("-");
}

void debugMsg (char* format, ...) {
    for (int i = 0; i < debugDepth; i++) {
        fputc('|', debugLog);
        //fputc(' ', debugLog);
    }

    va_list args;
    va_start(args, format);
    vfprintf(debugLog, format, args);
    va_end(args);

    fputc('\n', debugLog);
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
    fputs(str, debugLog);
}

void reportType (type DT) {
    char* Str = typeToStr(DT);
    fprintf(debugLog, "type: %s\n", Str);
    free(Str);
}

void reportSymbol (sym* Symbol) {
    /*Class*/

    fprintf(debugLog, "class: %s   ",
            symClassGetStr(Symbol->class));

    /*Symbol name*/

    fprintf(debugLog, "symbol: %s   ",
            Symbol->ident);

    /*Parent*/

    if (Symbol->class != symGlobal &&
        Symbol->class != symType &&
        Symbol->class != symStruct &&
        Symbol->class != symFunction)
        fprintf(debugLog, "parent: %s   ",
                Symbol->parent ? Symbol->parent->ident : "undefined");

    /*Type*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam ||
        Symbol->class == symFunction) {
        char* Str = typeToStr(Symbol->dt);
        fprintf(debugLog, "type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam) {
        if (Symbol->dt.array == 0)
            fprintf(debugLog, "size:   %d   ",
                    Symbol->dt.basic ? typeGetSize(Symbol->dt) : 0);

        else
            fprintf(debugLog, "size: %dx%d   ",
                    Symbol->dt.basic ? Symbol->dt.basic->size : 0,
                    Symbol->dt.array);

    } else if (Symbol->class == symStruct)
        fprintf(debugLog, "size:   %d   ",
                Symbol->size);

    /*Offset*/

    if (Symbol->class == symVar ||
        Symbol->class == symParam)
        fprintf(debugLog, "offset: %d   ",
               Symbol->offset);

    fputc('\n', debugLog);
}

void reportNode (ast* Node) {
    fprintf(debugLog, "node: %p   class: %s   next: %p   fc: %p   l: %p\n",
            (void*) Node,
            astClassGetStr(Node->class),
            (void*) Node->nextSibling,
            (void*) Node->firstChild,
            (void*) Node->l);
}

void reportRegs () {
    fprintf(debugLog, "[ ");

    for (int i = 0; i < regMax; i++)
        if (regIsUsed(i))
            fprintf(debugLog, "%s ", regToStr(i));

    fprintf(debugLog, "]\n");
}
