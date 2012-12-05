#include "stdarg.h"

#include "../inc/asm.h"

FILE* Asm;
int depth;

void asmInit (FILE* File) {
    Asm = File;
}

void asmOut (char* format, ...) {
    for (int i = 0; i < 4*depth; i++)
        fputc(' ', Asm);

    va_list args;
    va_start(args, format);
    asmVarOut(format, args);
    va_end(args);

    fputc('\n', Asm);
}

void asmVarOut (char* format, va_list args) {
    vfprintf(Asm, format, args);
}

void asmEnter () {
    depth++;
}

void asmLeave () {
    depth--;
}
