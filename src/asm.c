#include "../inc/asm.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/reg.h"

#include "stdlib.h"
#include "string.h"
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

    va_list args[2];
    va_start(args[0], format);
    va_copy(args[1], args[0]);
    debugVarMsg(format, args[0]);
    vfprintf(ctx->file, format, args[1]);
    va_end(args[1]);
    va_end(args[0]);

    fputc('\n', ctx->file);
    ctx->lineNo++;

    //fprintf(ctx->file, ".loc 1 %d 0\n", ctx->lineNo+1);
}

void asmEnter (asmCtx* ctx) {
    ctx->depth++;
}

void asmLeave (asmCtx* ctx) {
    if (ctx->depth > 0)
        ctx->depth--;
}
