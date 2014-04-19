#include "vector.h"
#include "ast.h"
#include "operand.h"

typedef struct sym sym;
typedef struct architecture architecture;
typedef struct asmCtx asmCtx;

typedef struct irBlock irBlock;

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
    termCallIndirect,
    termReturn
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
                sym* toAsSym;
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
    char* label;
    void* initial;
} irStaticData;

/*::::  ::::*/

typedef struct irBlock {
    vector/*<irInstr*>*/ instrs;
    irTerm* term;

    char* label;

    char* str;
    int length, capacity;

    ///Blocks that this block may (at runtime) have (directly)
    ///come from / go to, respectively
    vector/*<const irBlock*>*/ preds, succs;
} irBlock;

typedef struct irFn {
    char* name;
    irBlock *prologue, *entryPoint, *epilogue;
    vector/*<irBlock*>*/ blocks;
} irFn;

typedef struct irCtx {
    vector/*<irFn*>*/ fns;
    vector/*<irStaticData*>*/ sdata;

    int labelNo;

    asmCtx* asm;
    const architecture* arch;
} irCtx;

void irInit (irCtx* ctx, const char* output, const architecture* arch);
void irFree (irCtx* ctx);

void irEmit (irCtx* ctx);

irFn* irFnCreate (irCtx* ctx, const char* name);
irBlock* irBlockCreate (irCtx* ctx, irFn* fn);

void irBlockOut (irBlock* block, const char* format, ...);

operand irStringConstant (irCtx* ctx, const char* str);

/*:::: TERMINAL INSTRUCTIONS ::::*/

void irJump (irBlock* block, irBlock* to);
void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse);
void irCall (irBlock* block, sym* to, irBlock* ret);
void irCallIndirect (irBlock* block, operand to, irBlock* ret);
