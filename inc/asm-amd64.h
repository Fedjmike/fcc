#include "operand.h"

typedef struct asmCtx asmCtx;
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

void asmFnPrologue (asmCtx* ctx, const char* name, int localSize);
void asmFnEpilogue (asmCtx* ctx, operand labelEnd);

/**
 * Save and restore a register using the stack
 */
void asmSaveReg (irBlock* block, regIndex r);
void asmRestoreReg (irBlock* block, regIndex r);

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
void asmCall (irBlock* block, operand L);

void asmPush (irBlock* block, operand L);
void asmPop (irBlock* block, operand L);

void asmPushN (irBlock* block, int n);
void asmPopN (irBlock* block, int n);

void asmMove (irBlock* block, operand Dest, operand Src);
void asmConditionalMove (irBlock* block, operand Cond, operand Dest, operand Src);

operand asmWiden (irBlock* block, operand R, int size);
operand asmNarrow (irBlock* block, operand R, int size);

void asmEvalAddress (irBlock* block, operand L, operand R);

void asmCompare (irBlock* block, operand L, operand R);

void asmBOP (irBlock* block, boperation Op, operand L, operand R);

void asmDivision (irBlock* block, operand R);

void asmUOP (irBlock* block, uoperation Op, operand R);
