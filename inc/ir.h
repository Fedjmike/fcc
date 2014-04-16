#include "vector.h"
#include "operand.h"

typedef struct architecture architecture;

typedef enum irInstrTag {
    instrUndefined,
    instrJump,
    instrBranch,
    instrMove,
    instrBOP,
    instrUOP
} irInstrTag;

typedef enum irOpTag {
    irAdd
} irOpTag;

typedef struct irInstr {
    irInstrTag tag;
    irOpTag opTag;

    operand *result, *l, *r;
} irInstr;

typedef struct irBlock {
    vector/*<irInstr*>*/ instrs;
} irBlock;

typedef struct irCtx {
    vector/*<irBlock*>*/ blocks;
} irCtx;

void irInit (irCtx* ctx, const char* output, const architecture* arch);
void irFree (irCtx* ctx);

void irEmit (irCtx* ctx);

irBlock* irBlockCreate (irCtx* ir);
static void irBlockAdd (irBlock* block, irInstr* instr);

void irFnPrologue (irBlock* block, const char* name, int stacksize);
void irFnEpilogue (irBlock* block);

void irJump (irBlock* block, irBlock* to);
void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse);
