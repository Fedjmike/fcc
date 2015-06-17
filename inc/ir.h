#include "vector.h"
#include "ast.h"
#include "operand.h"

#include "stdint.h"

using "forward.h";

using "vector.h";
using "ast.h";
using "operand.h";

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
    dataRegular,
    dataStringConstant
} irStaticDataTag;

typedef struct irStaticData {
    irStaticDataTag tag;

    union {
        /*dataRegular*/
        struct {
            const char* label;
            bool global;
            int size;
            intptr_t initial;
        };
        /*dataStringConstant*/
        struct {
            char* strlabel;
            char* str;
        };
    };
} irStaticData;

/*::::  ::::*/

typedef struct irBlock {
    vector/*<irInstr*>*/ instrs;
    irTerm* term;

    char* label;

    char* str;
    int length, capacity;

    ///Index in the parent Fn's vector
    int nthChild;

    ///Blocks that this block may (at runtime) have (directly)
    ///come from / go to, respectively
    vector/*<const irBlock*>*/ preds, succs;
} irBlock;

typedef struct irFn {
    char* name;
    ///prologue and epilogue manage the stack frame and register saving
    ///created and managed internally. entryPoint is what the emitter should
    ///fill, using epilogue as a continuation / return point
    irBlock *prologue, *entryPoint, *epilogue;
    ///Includes and owns the above blocks, as well as all others
    vector/*<irBlock*>*/ blocks;
} irFn;

typedef struct irCtx {
    vector/*<irFn*>*/ fns;
    vector/*<irStaticData*>*/ data, rodata;

    int labelNo;

    asmCtx* asm;
    const architecture* arch;
} irCtx;

void irInit (irCtx* ctx, const char* output, const architecture* arch);
void irFree (irCtx* ctx);

void irEmit (irCtx* ctx);

/**If no name is provided, one will be allocated*/
irFn* irFnCreate (irCtx* ctx, const char* name, int stacksize);
irBlock* irBlockCreate (irCtx* ctx, irFn* fn);

void irBlockOut (irBlock* block, const char* format, ...);

/*:::: STATIC DATA ::::*/

void irStaticValue (irCtx* ctx, const char* label, bool global, int size, intptr_t initial);
operand irStringConstant (irCtx* ctx, const char* str);

/*:::: TERMINAL INSTRUCTIONS ::::*/

void irJump (irBlock* block, irBlock* to);
void irBranch (irBlock* block, operand cond, irBlock* ifTrue, irBlock* ifFalse);
void irCall (irBlock* block, sym* to, irBlock* ret);
void irCallIndirect (irBlock* block, operand to, irBlock* ret);

/*:::: ::::*/

int irBlockGetPredNo (irFn* fn, irBlock* block);
int irBlockGetSuccNo (irBlock* block);

/*:::: TRANSFORMATIONS ::::*/

void irBlockDelete (irFn* fn, irBlock* block);
void irBlocksCombine (irFn* fn, irBlock* pred, irBlock* succ);

/*:::: ::::*/

void irBlockLevelAnalysis (irCtx* ctx);
