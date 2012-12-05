#include "stdlib.h"
#include "stdio.h"

#include "../inc/debug.h"

FILE* debugLog;

void debugInit (FILE* Log) {
    debugLog = Log;
}

void report (char* Str) {
    fputs(Str, debugLog);
}

void reportType (type DT) {
    char* Str = typeToStr(DT);
    fprintf(debugLog, "type: %s\n", Str);
    free(Str);
}

void reportSymbol (sym* Symbol) {
    /*Symbol name*/

    fprintf(debugLog, "symbol: %s   ",
            Symbol->ident);

    /*Parent*/

    if (Symbol->class != symGlobal &&
        Symbol->class != symType &&
        Symbol->class != symStruct &&
        Symbol->class != symFunction)
        fprintf(debugLog, "parent: %s   ",
                Symbol->parent ? Symbol->parent->ident : 0);

    /*Type*/

    if (Symbol->class == symVar ||
        Symbol->class == symPara ||
        Symbol->class == symFunction) {
        char* Str = typeToStr(Symbol->dt);
        fprintf(debugLog, "type: %s   ", Str);
        free(Str);
    }

    /*Size*/

    if (Symbol->class == symVar ||
        Symbol->class == symPara) {
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
        Symbol->class == symPara)
        fprintf(debugLog, "offset: %d   ",
               Symbol->offset);

    fputc('\n', debugLog);
}

void reportNode (ast* Node) {
    fprintf(debugLog, "node: %p   class: %d   next: %p   fc: %p   l: %p\n",
            (void*) Node,
            Node->class,
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
