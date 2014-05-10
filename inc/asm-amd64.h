#include "operand.h"
#include "stdio.h"

typedef struct asmCtx asmCtx;
typedef struct irBlock irBlock;
typedef struct irCtx irCtx;
typedef enum regIndex regIndex;

typedef enum boperation {
    bopUndefined,
    bopAdd,
    bopSub,
    bopMul,
    bopBitAnd,
    bopBitOr,
    bopBitXor,
    bopShR,
    bopShL
} boperation;

typedef enum uoperation {
    uopUndefined,
    uopInc,
    uopDec,
    uopNeg,
    uopBitwiseNot
} uoperation;

void asmFilePrologue (asmCtx* ctx);
void asmFileEpilogue (asmCtx* ctx);

void asmFnLinkage (FILE* file, const char* name);

void asmFnPrologue (irCtx* ir, irBlock* block, int localSize);
void asmFnEpilogue (irCtx* ir, irBlock* block);

/**
 * Save and restore a register using the stack
 */
void asmSaveReg (irCtx* ir, irBlock* block, regIndex r);
void asmRestoreReg (irCtx* ir, irBlock* block, regIndex r);

void asmDataSection (asmCtx* ctx);
void asmRODataSection (asmCtx* ctx);

void asmStaticData (asmCtx* ctx, const char* label, bool global, int size, intptr_t initial);

/**
 * Place a string constant in the rodata section with the given label
 */
void asmStringConstant (asmCtx* ctx, const char* label, const char* str);

/**
 * Place a previously named label in the output
 */
void asmLabel (asmCtx* ctx, const char* label);

void asmJump (asmCtx* ctx, const char* label);
void asmBranch (asmCtx* ctx, operand Condition, const char* label);

/**
 * Call a function with the arguments currently on the stack
 */
void asmCall (asmCtx* ctx, const char* label);
void asmCallIndirect (irBlock* block, operand L);

void asmReturn (asmCtx* ctx);

void asmPush (irCtx* ir, irBlock* block, operand L);
void asmPop (irCtx* ir, irBlock* block, operand L);

void asmPushN (irCtx* ir, irBlock* block, int n);
void asmPopN (irCtx* ir, irBlock* block, int n);

void asmMove (irCtx* ir, irBlock* block, operand Dest, operand Src);
void asmConditionalMove (irCtx* ir, irBlock* block, operand Cond, operand Dest, operand Src);

void asmEvalAddress (irCtx* ir, irBlock* block, operand L, operand R);

void asmCompare (irCtx* ir, irBlock* block, operand L, operand R);

void asmBOP (irCtx* ir, irBlock* block, boperation Op, operand L, operand R);

void asmDivision (irCtx* ir, irBlock* block, operand R);

void asmUOP (irCtx* ir, irBlock* block, uoperation Op, operand R);
