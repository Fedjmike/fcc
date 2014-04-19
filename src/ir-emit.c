#include "../inc/ir.h"

#include "../inc/vector.h"
#include "../inc/sym.h"
#include "../inc/debug.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "stdlib.h"
#include "stdarg.h"

static void irEmitStaticData (irCtx* ctx, FILE* file, irStaticData* data);

static void irEmitFn (irCtx* ctx, FILE* file, irFn* fn);
static void irEmitBlock (irCtx* ctx, FILE* file, irBlock* block);
static void irEmitTerm (irCtx* ctx, FILE* file, irTerm* term);

void irEmit (irCtx* ctx) {
    FILE* file = ctx->asm->file;

    asmFilePrologue(ctx->asm);

    for (int i = 0; i < ctx->fns.length; i++) {
        irFn* fn = vectorGet(&ctx->fns, i);
        irEmitFn(ctx, file, fn);
    }

    for (int i = 0; i < ctx->sdata.length; i++) {
        irStaticData* data = vectorGet(&ctx->sdata, i);
        irEmitStaticData(ctx, file, data);
    }

    asmFileEpilogue(ctx->asm);
}

static void irEmitStaticData (irCtx* ctx, FILE* file, irStaticData* data) {
    (void) file;

    if (data->tag == dataStringConstant)
        asmStringConstant(ctx->asm, data->label, (char*) data->initial);

    else
        debugErrorUnhandledInt("irEmitStaticData", "static data tag", data->tag);
}

static void irEmitFn (irCtx* ctx, FILE* file, irFn* fn) {
    debugEnter(fn->name);

    asmFnLinkage(file, fn->name);

    /*Constituent blocks, including prologue and epilogue*/
    for (int j = 0; j < fn->blocks.length; j++) {
        irBlock* block = vectorGet(&fn->blocks, j);
        irEmitBlock(ctx, file, block);
    }

    debugLeave();
}

static void irEmitBlock (irCtx* ctx, FILE* file, irBlock* block) {
    (void) ctx;

    debugEnter(block->label);

    asmLabel(ctx->asm, block->label);
    fputs(block->str, file);
    debugMsg(block->str);

    if (block->term)
        irEmitTerm(ctx, file, block->term);

    else
        debugError("irEmitBlock", "unterminated block %s", block->label);

    fputs("\n", file);

    debugLeave();
}

static void irEmitTerm (irCtx* ctx, FILE* file, irTerm* term) {
    (void) file;

    if (term->tag == termJump)
        asmJump(ctx->asm, term->to->label);

    else if (term->tag == termBranch) {
        asmBranch(ctx->asm, term->cond, term->ifFalse->label);
        asmJump(ctx->asm, term->ifTrue->label);

    } else if (term->tag == termCall) {
        asmCall(ctx->asm, term->toAsSym->ident);
        asmJump(ctx->asm, term->ret->label);

    } else if (term->tag == termCallIndirect) {
        asmCallIndirect(ctx->asm, term->toAsOperand);

    } else if (term->tag == termReturn)
        asmReturn(ctx->asm);

    else
        debugErrorUnhandledInt("irEmitTerm", "terminal tag", term->tag);
}
