#include "../inc/ir.h"

#include "../inc/vector.h"
#include "../inc/debug.h"
#include "../inc/operand.h"
#include "../inc/asm.h"

#include "stdlib.h"
#include "stdarg.h"

static void irAddFn (irCtx* ctx, irFn* fn);
static void irAddStaticData (irCtx* ctx, irStaticData* sdata);
static void irAddBlock (irFn* fn, irBlock* block);
static void irAddInstr (irBlock* block, irInstr* instr);
static void irTerminateBlock (irBlock* block, irTerm* term);

static irStaticData* irStaticDataCreate (irCtx* ctx, irStaticDataTag tag);
static void irStaticDataDestroy (irStaticData* data);

static void irFnDestroy (irFn* fn);
static void irBlockDestroy (irBlock* block);

static irInstr* irInstrCreate (irInstrTag tag, irBlock* block);
static void irInstrDestroy (irInstr* instr);

static irTerm* irTermCreate (irTermTag tag, irBlock* block);
static void irTermDestroy (irTerm* term);

static void irReturn (irBlock* block);

enum {
    irCtxFnNo = 8,
    irCtxSDataNo = 8,
    irFnBlockNo = 8,
    irBlockInstrNo = 8,
    irBlockStrSize = 1024
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

void irEmit (irCtx* ctx);

static void irAddFn (irCtx* ctx, irFn* fn) {
    vectorPush(&ctx->fns, fn);
}

static void irAddStaticData (irCtx* ctx, irStaticData* sdata) {
    vectorPush(&ctx->sdata, sdata);
}

static char* irCreateLabel (irCtx* ctx) {
    char* label = malloc(10);
    sprintf(label, ".%X", ctx->labelNo++);
    return label;
}

/*:::: FUNCTION INTERNALS ::::*/

irFn* irFnCreate (irCtx* ctx, const char* name, int stacksize) {
    irFn* fn = malloc(sizeof(irFn));
    fn->name = strdup(name);
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
    vectorPush(&fn->blocks, block);
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

    irAddBlock(fn, block);

    return block;
}

static void irBlockDestroy (irBlock* block) {
    vectorFreeObjs(&block->instrs, (vectorDtor) irInstrDestroy);
    irTermDestroy(block->term);

    free(block->label);
    free(block->str);
    free(block);
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

static void irTerminateBlock (irBlock* block, irTerm* term) {
    if (block->term) {
        debugError("irTerminateBlock", "attempted to terminate already terminated block");
        irTermDestroy(block->term);
    }

    block->term = term;
}

/*:::: STATIC DATA INTERNALS ::::*/

static irStaticData* irStaticDataCreate (irCtx* ctx, irStaticDataTag tag) {
    irStaticData* data = malloc(sizeof(irStaticData));
    data->tag = tag;

    irAddStaticData(ctx, data);

    return data;
}

static void irStaticDataDestroy (irStaticData* data) {
    free(data->initial);
    free(data);
}

/*:::: STATIC DATA ::::*/

operand irStringConstant (irCtx* ctx, const char* str) {
    irStaticData* data = irStaticDataCreate(ctx, dataStringConstant);
    data->initial = (void*) strdup(str);

    //TODO
    return operandCreateLabelOffset(strdup(""));
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

    irTerminateBlock(block, term);

    return term;
}

static void irTermDestroy (irTerm* term) {
    free(term);
}

/*:::: TERMINAL INSTRUCTIONS ::::*/

void irJump (irBlock* block, irBlock* to) {
    irTerm* term = irTermCreate(termJump, block);
    term->to = to;
}

void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse) {
    irTerm* term = irTermCreate(termBranch, block);
    term->cond = cond;
    term->ifTrue = ifTrue;
    term->ifFalse = ifFalse;
}

void irCall (irBlock* block, sym* to, irBlock* ret) {
    irTerm* term = irTermCreate(termCall, block);
    term->toAsSym = to;
    term->ret = ret;
}

void irCallIndirect (irBlock* block, operand to, irBlock* ret) {
    irTerm* term = irTermCreate(termCallIndirect, block);
    term->toAsOperand = to;
    term->ret = ret;
}

static void irReturn (irBlock* block) {
    irTermCreate(termReturn, block);
}
