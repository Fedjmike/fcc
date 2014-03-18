#include "../inc/asm.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/reg.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"

asmCtx* asmInit (const char* output, const architecture* arch) {
    asmCtx* ctx = malloc(sizeof(asmCtx));
    ctx->filename = strdup(output);
    ctx->file = fopen(output, "w");
    ctx->lineNo = 1;
    ctx->depth = 0;
    ctx->arch = arch;
    ctx->stackPtr = operandCreateReg(regRequest(regRSP, arch->wordsize));
    ctx->basePtr = operandCreateReg(regRequest(regRBP, arch->wordsize));
    return ctx;
}

void asmEnd (asmCtx* ctx) {
    free(ctx->filename);
    fclose(ctx->file);
    operandFree(ctx->stackPtr);
    operandFree(ctx->basePtr);
    free(ctx);
}

void asmOutLn (asmCtx* ctx, char* format, ...) {
    for (int i = 0; i < 4*ctx->depth; i++)
        fputc(' ', ctx->file);

    va_list args;
    va_start(args, format);
    debugVarMsg(format, args);
    vfprintf(ctx->file, format, args);
    va_end(args);

    fputc('\n', ctx->file);
    ctx->lineNo++;

    if (ctx->lineNo % 2 == 0)
        asmOutLn(ctx, ".loc 1 %d 0", ctx->lineNo+1);
}

void asmEnter (asmCtx* ctx) {
    ctx->depth++;
}

void asmLeave (asmCtx* ctx) {
    ctx->depth--;
}
