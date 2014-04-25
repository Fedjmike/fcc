#include "../inc/ir.h"

#include "../inc/vector.h"
#include "../inc/hashmap.h"
#include "../inc/sym.h"
#include "../inc/debug.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "stdlib.h"
#include "stdarg.h"

static void irEmitStaticData (irCtx* ctx, FILE* file, const irStaticData* data);

static void irEmitFn (irCtx* ctx, FILE* file, const irFn* fn);
static void irEmitBlock (irCtx* ctx, FILE* file, const irBlock* block, const irBlock* nextblock);
static void irEmitTerm (irCtx* ctx, FILE* file, const irTerm* term, const irBlock* nextblock);

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

static void irEmitStaticData (irCtx* ctx, FILE* file, const irStaticData* data) {
    (void) file;

    if (data->tag == dataStringConstant)
        asmStringConstant(ctx->asm, data->label, (char*) data->initial);

    else
        debugErrorUnhandledInt("irEmitStaticData", "static data tag", data->tag);
}

static void irEmitBlockChain (irCtx* ctx, FILE* file, intset* done, vector* priority, const irBlock* block) {
    /*Already put in the priority list, leave*/
    if (intsetAdd(done, (uintptr_t) block))
        return;

    printf("master: %s\n", block->label);

    /*Add all the predecessors and their predecessors to the list*/
    for (int j = 0; j < block->preds.length; j++) {
        irBlock* pred = vectorGet(&block->preds, j);
        printf("chain: %s\n", pred->label);
        irEmitBlockChain(ctx, file, done, priority, pred);
    }

    /*Followed by this block*/
    vectorPush(priority, (void*) block);
}

static void irEmitFn (irCtx* ctx, FILE* file, const irFn* fn) {
    debugEnter(fn->name);

    asmFnLinkage(file, fn->name);

    intset done;
    intsetInit(&done, fn->blocks.length*2);

    vector priority;
    vectorInit(&priority, fn->blocks.length);

    /*Decide an order to emit the blocks in to minimize unnecessary jumps*/
    irEmitBlockChain(ctx, file, &done, &priority, fn->epilogue);

    /*Order decided, emit*/
    for (int j = 0; j < priority.length; j++) {
        irBlock *block = vectorGet(&priority, j),
                *nextblock = vectorGet(&priority, j+1);
        irEmitBlock(ctx, file, block, nextblock);
    }

    debugLeave();
}

static void irEmitBlock (irCtx* ctx, FILE* file, const irBlock* block, const irBlock* nextblock) {
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

static void irEmitTerm (irCtx* ctx, FILE* file, const irTerm* term, const irBlock* nextblock) {
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

    } else if (term->tag == termCallIndirect) {
        asmCallIndirect(ctx->asm, term->toAsOperand);
        jumpTo = term->ret;

    } else if (term->tag == termReturn)
        asmReturn(ctx->asm);

    else
        debugErrorUnhandledInt("irEmitTerm", "terminal tag", term->tag);

    /*Perform the jump if not redundant*/
    if (jumpTo && jumpTo != nextblock)
        asmJump(ctx->asm, jumpTo->label);
}
