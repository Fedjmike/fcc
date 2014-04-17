#include "vector.h"
#include "ast.h"
#include "operand.h"

typedef struct architecture architecture;
typedef struct asmCtx asmCtx;

typedef struct irBlock irBlock;
typedef struct irFn irFn;

/*::::  ::::*/

typedef enum irInstrTag {
    instrUndefined,
    instrMove,
    instrBOP,
    instrUOP
} irInstrTag;

typedef struct irInstr {
    irInstrTag tag;
    opTag op;

    operand dest, l, r;
} irInstr;

/*::::  ::::*/

typedef enum irTermTag {
    termUndefined,
    termJump,
    termBranch,
    termCall,
    termCallIndirect
} irTermTag;

typedef struct irTerm {
    irTermTag tag;

    union {
        /*termJump*/
        irBlock* to;
        /*termBranch*/
        struct {
            irBlock *ifTrue, *ifFalse;
            operand cond;
        };
        /*termCall termCallIndirect*/
        struct {
            irBlock* ret;
            union {
                /*termCall*/
                irFn* toAsFn;
                /*termCallIndirect*/
                operand toAsOperand;
            };
        };
    };
} irTerm;

/*::::  ::::*/

typedef enum irStaticDataTag {
    dataUndefined,
    dataStringConstant
} irStaticDataTag;

typedef struct irStaticData {
    irStaticDataTag tag;

    void* initial;
} irStaticData;

/*::::  ::::*/

typedef struct irBlock {
    vector/*<irInstr*>*/ instrs;
    irTerm* term;
} irBlock;

typedef struct irFn {
    irBlock *prologue, *epilogue;
    vector/*<irBlock*>*/ blocks;
} irFn;

typedef struct irCtx {
    vector/*<irFn*>*/ fns;
    vector/*<irStaticData*>*/ sdata;

    asmCtx* asm;
    const architecture* arch;
} irCtx;

void irInit (irCtx* ctx, const char* output, const architecture* arch);
void irFree (irCtx* ctx);

void irEmit (irCtx* ctx);

irFn* irFnCreate (irCtx* ctx);
irBlock* irBlockCreate (irCtx* ctx, irFn* fn);

operand irStringConstant (irCtx* ctx, const char* str);

/*:::: TERMINAL INSTRUCTIONS ::::*/

void irJump (irBlock* block, irBlock* to);
void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse);
void irCall (irBlock* block, irFn* to, irBlock* ret);
void irCallIndirect (irBlock* block, operand to, irBlock* ret);
