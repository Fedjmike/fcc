#include "vector.h"
#include "ast.h"
#include "operand.h"

typedef struct architecture architecture;
typedef struct asmCtx asmCtx;

typedef enum irInstrTag {
    instrUndefined,
    instrJump,
    instrBranch,
    instrMove,
    instrBOP,
    instrUOP
} irInstrTag;

typedef struct irInstr {
    irInstrTag tag;
    opTag op;

    operand dest, l, r;
} irInstr;

typedef struct irBlock {
    vector/*<irInstr*>*/ instrs;
} irBlock;

typedef struct irFn {
    vector/*<irBlock*>*/ blocks;
} irFn;

typedef struct irCtx {
    vector/*<irFn*>*/ fns;
    irFn* curFn;

    asmCtx* asm;
    const architecture* arch;
} irCtx;

void irInit (irCtx* ctx, const char* output, const architecture* arch);
void irFree (irCtx* ctx);

void irEmit (irCtx* ctx);

irBlock* irBlockCreate (irCtx* ir);
static void irBlockAdd (irBlock* block, irInstr* instr);

operand irStringConstant (irCtx* ir, const char* str);

void irFnPrologue (irBlock* block, const char* name, int stacksize);
void irFnEpilogue (irBlock* block);

/*:::: BLOCK TERMINATING/LINKING INSTRUCTIONS ::::*/

void irJump (irBlock* block, irBlock* to);
void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse);
void irCall (irBlock* block, irFn* to, irBlock* ret);
void irCallIndirect (irBlock* block, operand to, irBlock* ret);
