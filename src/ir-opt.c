#include "../inc/ir.h"

#include "../inc/hashmap.h"

#include "stdio.h"

using "../inc/ir.h";

using "../inc/hashmap.h";

using "stdio.h";

static void blaFn (irFn* fn);
static bool blaBlock (irFn* fn, intset/*<irBlock*>*/* done, irBlock* block);

static bool ubrBlock (irFn* fn, irBlock* block);
static bool lbcBlock (irFn* fn, irBlock* block);

/*Block Level Analysis (BLA) involves two optimizations:
    1. Unreachable Block Removal (UBR)
        - Blocks with no predecessors are removed.
    2. Linear Block Combination (LBC)
        - Pairs of blocks with only each other as predecessor/successor
          are combined into one.
        - Transitively, this means linear chains of blocks are combined.

  UBR informs LBC, consider:
    - A removed block may reduce another block to only one predecessor
        --A---C--
             /
          B-/

  The reverse is not true. Therefore UBR is done first, then LBC.*/

void irBlockLevelAnalysis (irCtx* ctx) {
    for (int i = 0; i < ctx->fns.length; i++) {
        irFn* fn = vectorGet(&ctx->fns, i);
        blaFn(fn);
    }
}

static void blaFn (irFn* fn) {
    intset/*<irBlock*>*/ done;
    intsetInit(&done, fn->blocks.length);

    blaBlock(fn, &done, fn->epilogue);

    intsetFree(&done);
}

static bool blaBlock (irFn* fn, intset/*<irBlock*>*/* done, irBlock* block) {
    /*Avoid cycles*/
    if (intsetAdd(done, (intptr_t) block))
        return false;

    /*Recursively analyze any preds*/
    for (int i = 0; i < block->preds.length; i++) {
        irBlock* pred = vectorGet(&block->preds, i);
        bool deleted = blaBlock(fn, done, pred);

        if (deleted)
            i--;
    }

    return ubrBlock(fn, block) || lbcBlock(fn, block);
}

static bool ubrBlock (irFn* fn, irBlock* block) {
    /*No predecessors => unreachable code => delete*/
    if (irBlockGetPredNo(fn, block) == 0) {
        irBlockDelete(fn, block);
        return true;
    }

    return false;
}

static bool lbcBlock (irFn* fn, irBlock* block) {
    irBlock *pred = vectorGet(&block->preds, 0);

    /*Only one predecessor, and we're its only successor
      => combine*/
    if (   block->preds.length == 1
        && irBlockGetSuccNo(pred) == 1) {
        irBlocksCombine(fn, pred, block);
        return true;
    }

    return false;
}
