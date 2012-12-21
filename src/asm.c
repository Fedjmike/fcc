#include "../inc/asm.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"

asmCtx* asmInit (FILE* File) {
    asmCtx* ctx = malloc(sizeof(asmCtx));
    ctx->file = File;
    ctx->depth = 0;
    return ctx;
}
void asmEnd (asmCtx* ctx) {
    free(ctx);
}

void asmOutLn (asmCtx* ctx, char* format, ...) {
    for (int i = 0; i < 4*ctx->depth; i++)
        fputc(' ', ctx->file);

    va_list args;
    va_start(args, format);
    asmVarOut(ctx, format, args);
    va_end(args);

    fputc('\n', ctx->file);
}

void asmVarOut (asmCtx* ctx, char* format, va_list args) {
    vfprintf(ctx->file, format, args);
}

void asmEnter (asmCtx* ctx) {
    ctx->depth++;
}

void asmLeave (asmCtx* ctx) {
    ctx->depth--;
}
