#include "../inc/ir.h"

#include "../inc/vector.h"
#include "../inc/debug.h"
#include "../inc/operand.h"
#include "../inc/asm.h"
#include "../inc/asm-amd64.h"

#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

static void irAddFn (irCtx* ctx, irFn* fn);
static void irAddStaticData (irCtx* ctx, irStaticData* sdata);

/*:::: ::::*/

static irStaticData* irStaticDataCreate (irCtx* ctx, irStaticDataTag tag);
static void irStaticDataDestroy (irStaticData* data);

/*:::: ::::*/

static void irFnDestroy (irFn* fn);
static void irAddBlock (irFn* fn, irBlock* block);

/*:::: ::::*/

static void irBlockDestroy (irBlock* block);
static void irAddInstr (irBlock* block, irInstr* instr);
static void irBlockTerminate (irBlock* block, irTerm* term);

/**
 * Update the preds/succs vectors in from and to
 */
static void irBlockLink (irBlock* from, irBlock* to);

/*:::: ::::*/

static irInstr* irInstrCreate (irInstrTag tag, irBlock* block);
static void irInstrDestroy (irInstr* instr);

/*:::: ::::*/

static irTerm* irTermCreate (irTermTag tag, irBlock* block);
static void irTermDestroy (irTerm* term);

/*:::: ::::*/

static void irReturn (irBlock* block);

/*Constants for various initializations
  Initial sizes of vectors, strings and such*/
enum {
    irCtxFnNo = 8,
    irCtxSDataNo = 8,
    irFnBlockNo = 8,
    irBlockInstrNo = 8,
    irBlockStrSize = 1024,
    irBlockPredNo = 2,
    irBlockSuccNo = 2
};

/*:::: IR CONTEXT ::::*/

void irInit (irCtx* ctx, const char* output, const architecture* arch) {
    vectorInit(&ctx->fns, irCtxFnNo);
    vectorInit(&ctx->sdata, irCtxSDataNo);

    ctx->labelNo = 0;

    ctx->asm = asmInit(output, arch);
    ctx->arch = arch;
}

void irFree (irCtx* ctx) {
    vectorFreeObjs(&ctx->fns, (vectorDtor) irFnDestroy);
    vectorFreeObjs(&ctx->sdata, (vectorDtor) irStaticDataDestroy);
    asmEnd(ctx->asm);
}

static void irAddFn (irCtx* ctx, irFn* fn) {
    vectorPush(&ctx->fns, fn);
}

static void irAddStaticData (irCtx* ctx, irStaticData* sdata) {
    vectorPush(&ctx->sdata, sdata);
}

static char* irCreateLabel (irCtx* ctx) {
    char* label = malloc(10);
    sprintf(label, ".%04X", ctx->labelNo++);
    return label;
}

/*:::: FUNCTION INTERNALS ::::*/

irFn* irFnCreate (irCtx* ctx, const char* name, int stacksize) {
    irFn* fn = malloc(sizeof(irFn));
    fn->name = name ? strdup(name) : irCreateLabel(ctx);
    vectorInit(&fn->blocks, irFnBlockNo);

    /*These will get added to fn->blocks, which now owns them*/
    fn->prologue = irBlockCreate(ctx, fn);
    fn->entryPoint = irBlockCreate(ctx, fn);
    fn->epilogue = irBlockCreate(ctx, fn);

    asmFnPrologue(ctx, fn->prologue, stacksize);
    asmFnEpilogue(ctx, fn->epilogue);

    irJump(fn->prologue, fn->entryPoint);
    irReturn(fn->epilogue);

    irAddFn(ctx, fn);

    return fn;
}

static void irFnDestroy (irFn* fn) {
    vectorFreeObjs(&fn->blocks, (vectorDtor) irBlockDestroy);
    free(fn->name);
    free(fn);
}

static void irAddBlock (irFn* fn, irBlock* block) {
    block->nthChild = vectorPush(&fn->blocks, block);
}

/*:::: BLOCK INTERNALS ::::*/

irBlock* irBlockCreate (irCtx* ctx, irFn* fn) {
    irBlock* block = malloc(sizeof(irBlock));
    vectorInit(&block->instrs, irBlockInstrNo);
    block->term = 0;
    block->label = irCreateLabel(ctx);

    block->str = calloc(irBlockStrSize, sizeof(char*));
    block->length = 0;
    block->capacity = irBlockStrSize;

    vectorInit(&block->preds, irBlockPredNo);
    vectorInit(&block->succs, irBlockSuccNo);

    irAddBlock(fn, block);

    return block;
}

static void irBlockDestroy (irBlock* block) {
    vectorFree(&block->preds);
    vectorFree(&block->succs);

    vectorFreeObjs(&block->instrs, (vectorDtor) irInstrDestroy);
    irTermDestroy(block->term);

    free(block->label);
    free(block->str);
    free(block);
}

int irBlockGetPredNo (irFn* fn, irBlock* block) {
    return block->preds.length + (block == fn->prologue ? 1 : 0);
}

int irBlockGetSuccNo (irBlock* block) {
    return block->succs.length + (block->term->tag == termCall || block->term->tag == termCallIndirect ? 1 : 0);
}

void irBlockOut (irBlock* block, const char* format, ...) {
    va_list args[2];
    va_start(args[0], format);
    va_copy(args[1], args[0]);
    debugVarMsg(format, args[1]);
    va_end(args[1]);

    int length = vsnprintf(block->str+block->length, block->capacity, format, args[0]);
    va_end(args[0]);

    if (length < 0 || block->length+length >= block->capacity) {
        block->capacity *= 2;
        block->capacity += length+2;
        block->str = realloc(block->str, block->capacity);

        va_start(args[0], format);
        vsnprintf(block->str+block->length, block->capacity, format, args[0]);
        va_end(args[0]);
    }

    block->length += length;
    block->str[block->length++] = '\n';
}

static void irAddInstr (irBlock* block, irInstr* instr) {
    vectorPush(&block->instrs, instr);
}

static void irBlockTerminate (irBlock* block, irTerm* term) {
    if (block->term) {
        debugError("irBlockTerminate", "attempted to terminate already terminated block");
        irTermDestroy(block->term);
    }

    block->term = term;
}

static void irBlockLink (irBlock* from, irBlock* to) {
    vectorPush(&from->succs, to);
    vectorPush(&to->preds, from);
}

/*:::: STATIC DATA INTERNALS ::::*/

static irStaticData* irStaticDataCreate (irCtx* ctx, irStaticDataTag tag) {
    irStaticData* data = malloc(sizeof(irStaticData));
    data->tag = tag;
    data->label = irCreateLabel(ctx);

    irAddStaticData(ctx, data);

    return data;
}

static void irStaticDataDestroy (irStaticData* data) {
    free(data->initial);
    free(data->label);
    free(data);
}

/*:::: STATIC DATA ::::*/

operand irStringConstant (irCtx* ctx, const char* str) {
    irStaticData* data = irStaticDataCreate(ctx, dataStringConstant);
    data->initial = (void*) strdup(str);

    return operandCreateLabelOffset(data->label);
}

/*:::: INSTRUCTION INTERNALS ::::*/

static irInstr* irInstrCreate (irInstrTag tag, irBlock* block) {
    irInstr* instr = malloc(sizeof(irInstr));
    instr->tag = tag;
    instr->op = opUndefined;

    instr->dest = operandCreate(operandUndefined);
    instr->l = operandCreate(operandUndefined);
    instr->r = operandCreate(operandUndefined);

    irAddInstr(block, instr);

    return instr;
}

static void irInstrDestroy (irInstr* instr) {
    (void) irInstrCreate;
    free(instr);
}

/*:::: TERMINAL INSTRUCTION INTERNALS ::::*/

static irTerm* irTermCreate (irTermTag tag, irBlock* block) {
    irTerm* term = malloc(sizeof(irTerm));
    term->tag = tag;

    term->to = 0;
    term->cond = operandCreate(operandUndefined);
    term->ifTrue = 0;
    term->ifFalse = 0;
    term->ret = 0;
    term->toAsSym = 0;
    term->toAsOperand = operandCreate(operandUndefined);

    irBlockTerminate(block, term);

    return term;
}

static void irTermDestroy (irTerm* term) {
    free(term);
}

/*:::: TERMINAL INSTRUCTIONS ::::*/

void irJump (irBlock* block, irBlock* to) {
    irTerm* term = irTermCreate(termJump, block);
    term->to = to;

    irBlockLink(block, to);
}

void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse) {
    irTerm* term = irTermCreate(termBranch, block);
    term->cond = cond;
    term->ifTrue = ifTrue;
    term->ifFalse = ifFalse;

    irBlockLink(block, ifTrue);
    irBlockLink(block, ifFalse);
}

void irCall (irBlock* block, sym* to, irBlock* ret) {
    irTerm* term = irTermCreate(termCall, block);
    term->toAsSym = to;
    term->ret = ret;

    irBlockLink(block, ret);
}

void irCallIndirect (irBlock* block, operand to, irBlock* ret) {
    irTerm* term = irTermCreate(termCallIndirect, block);
    term->toAsOperand = to;
    term->ret = ret;

    irBlockLink(block, ret);

    /*Hack! Emit the first part of the call now while the operand and any regs
      involved are still valid.*/
    asmCallIndirect(block, to);
}

static void irReturn (irBlock* block) {
    irTermCreate(termReturn, block);
}

/*:::: TRANSFORMATIONS ::::*/

void irBlockDelete (irFn* fn, irBlock* block) {
    /*Remove it from the fn's vector*/
    irBlock* replacement = vectorRemoveReorder(&fn->blocks, block->nthChild);
    replacement->nthChild = block->nthChild;

    /*Remove it from any preds and succs*/

    for (int i = 0; i < block->preds.length; i++) {
        irBlock* pred = vectorGet(&block->preds, i);

        int index = vectorFind(&pred->succs, block);
        vectorRemoveReorder(&pred->succs, index);
    }

    for (int i = 0; i < block->succs.length; i++) {
        irBlock* succ = vectorGet(&block->succs, i);

        int index = vectorFind(&succ->preds, block);
        vectorRemoveReorder(&succ->preds, index);
    }

    irBlockDestroy(block);
}

void irBlocksCombine (irFn* fn, irBlock* pred, irBlock* succ) {
    /*Combine them by putting everything from the succ into the pred,
      taking ownership where possible. Then destroy the succ.*/

    /*Cat the instr vectors*/
    vectorPushFromVector(&pred->instrs, &succ->instrs);

    /*Free the pred's terminal and take the succ's*/
    irTermDestroy(pred->term);
    pred->term = succ->term;
    succ->term = 0;

    /*Cat the strings*/

    int totalLength = pred->length + succ->length-1;

    if (pred->capacity >= totalLength) {
        strncat(pred->str, succ->str, pred->capacity);
        pred->length = totalLength;

    } else {
        char* str = malloc(totalLength);
        snprintf(str, totalLength, "%s%s", pred->str, succ->str);
        free(pred->str);
        pred->str = str;
    }

    /*Link to the succs of the succ*/
    for (int i = 0; i < succ->succs.length; i++)
        irBlockLink(pred, vectorGet(&succ->succs, i));

    /*Make sure there is always a valid epilogue*/
    if (fn->epilogue == succ)
        fn->epilogue = pred;

    irBlockDelete(fn, succ);
}
