#include "../inc/ir.h"

static bool ubrBlock (irFn* fn, irBlock* block);
static bool lbcBlock (irFn* fn, irBlock* block);

/*Block Level Analysis (BLA) involves two optimizations:
    1. Unreachable Block Removal (DBR)
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

        /*Unreachable Block Removal*/
        for (int j = 0; j < fn->blocks.length; j++) {
            irBlock *block = vectorGet(&fn->blocks, j);
            bool deleted = ubrBlock(fn, block);

            if (deleted)
                j = 0;
        }

        /*Linear Block Combination*/
        for (int j = 0; j < fn->blocks.length; j++) {
            irBlock *block = vectorGet(&fn->blocks, j);
            bool deleted = lbcBlock(fn, block);

            if (deleted)
                j = 0;
        }
    }
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
    irBlock *pred = vectorGet(&block->preds, 0),
            *succ = vectorGet(&block->succs, 0);

    /*Only one successor, and we're its only predecessor
      => combine*/
    if (   irBlockGetSuccNo(block) == 1
        && irBlockGetPredNo(fn, succ) == 1) {
        irBlocksCombine(fn, block, succ);
        return true;
    }

    /*Same deal with our predecessor*/
    if (   irBlockGetPredNo(fn, block) == 1
        && irBlockGetSuccNo(pred) == 1) {
        irBlocksCombine(fn, pred, block);
        return true;
    }

    return false;
}
