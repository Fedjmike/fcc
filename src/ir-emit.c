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
static void irEmitBlock (irCtx* ctx, FILE* file, irBlock* block, irBlock* nextblock);
static void irEmitTerm (irCtx* ctx, FILE* file, irTerm* term, irBlock* nextblock);

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
        irBlock *block = vectorGet(&fn->blocks, j),
                *nextblock = vectorGet(&fn->blocks, j+1);
        irEmitBlock(ctx, file, block, nextblock);
    }

    debugLeave();
}

static void irEmitBlock (irCtx* ctx, FILE* file, irBlock* block, irBlock* nextblock) {
    debugEnter(block->label);

    asmLabel(ctx->asm, block->label);
    fputs(block->str, file);
    debugMsg(block->str);

    if (block->term)
        irEmitTerm(ctx, file, block->term, nextblock);

    else
        debugError("irEmitBlock", "unterminated block %s", block->label);

    fputs("\n", file);

    debugLeave();
}

static void irEmitTerm (irCtx* ctx, FILE* file, irTerm* term, irBlock* nextblock) {
    (void) file;

    /*Some of the terminals end in a jump
      Use this to unify the jump logic*/
    irBlock* jumpTo = 0;

    if (term->tag == termJump)
        jumpTo = term->to;

    else if (term->tag == termBranch) {
        asmBranch(ctx->asm, term->cond, term->ifFalse->label);
        jumpTo = term->ifTrue;

    } else if (term->tag == termCall) {
        asmCall(ctx->asm, term->toAsSym->ident);
        jumpTo = term->ret;

    } else if (term->tag == termCallIndirect)
        asmCallIndirect(ctx->asm, term->toAsOperand);

    else if (term->tag == termReturn)
        asmReturn(ctx->asm);

    else
        debugErrorUnhandledInt("irEmitTerm", "terminal tag", term->tag);

    /*Perform the jump if not redundant*/
    if (jumpTo && jumpTo != nextblock)
        asmJump(ctx->asm, jumpTo->label);
}
