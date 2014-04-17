#include "operand.h"

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

void asmFnPrologue (irCtx* ir, irBlock* block, const char* name, int localSize);
void asmFnEpilogue (irCtx* ir, irBlock* block, operand labelEnd);

/**
 * Save and restore a register using the stack
 */
void asmSaveReg (irCtx* ir, irBlock* block, regIndex r);
void asmRestoreReg (irCtx* ir, irBlock* block, regIndex r);

/**
 * Place a string constant in the rodata section with the given label
 */
void asmStringConstant (asmCtx* ctx, operand label, const char* str);

/**
 * Place a previously named label in the output
 */
void asmLabel (asmCtx* ctx, operand L);

void asmJump (asmCtx* ctx, operand L);
void asmBranch (asmCtx* ctx, operand Condition, operand L);

/**
 * Call a function with the arguments currently on the stack
 */
void asmCall (irCtx* ir, irBlock* block, operand L);

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
