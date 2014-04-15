#include "operand.h"

using "forward.h";

using "operand.h";

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
void asmSaveReg (asmCtx* ctx, regIndex r);
void asmRestoreReg (asmCtx* ctx, regIndex r);

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
void asmCall (asmCtx* ctx, operand L);

void asmPush (asmCtx* ctx, operand L);
void asmPop (asmCtx* ctx, operand L);

void asmPushN (asmCtx* ctx, int n);
void asmPopN (asmCtx* ctx, int n);

void asmMove (asmCtx* ctx, operand Dest, operand Src);
void asmConditionalMove (asmCtx* ctx, operand Cond, operand Dest, operand Src);

operand asmWiden (asmCtx* ctx, operand R, int size);
operand asmNarrow (asmCtx* ctx, operand R, int size);

void asmEvalAddress (asmCtx* ctx, operand L, operand R);

void asmCompare (asmCtx* ctx, operand L, operand R);

void asmBOP (asmCtx* ctx, boperation Op, operand L, operand R);

void asmDivision (asmCtx* ctx, operand R);

void asmUOP (asmCtx* ctx, uoperation Op, operand R);
