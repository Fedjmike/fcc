#include "operand.h"

struct asmCtx;

typedef enum {
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

typedef enum {
	uopUndefined,
	uopInc,
	uopDec,
	uopNeg,
	uopBitwiseNot
} uoperation;

void asmFilePrologue (struct asmCtx* ctx);
void asmFileEpilogue (struct asmCtx* ctx);

void asmFnPrologue (struct asmCtx* ctx, operand Name, int localSize);
void asmFnEpilogue (struct asmCtx* ctx, operand EndLabel);

/**
 * Place a string constant in the rodata section with the given label
 */
void asmStringConstant (struct asmCtx* ctx, operand label, const char* str);

/**
 * Place a previously named label in the output
 */
void asmLabel (struct asmCtx* ctx, operand L);

void asmJump (struct asmCtx* ctx, operand L);
void asmBranch (struct asmCtx* ctx, operand Condition, operand L);

void asmPush (struct asmCtx* ctx, operand L);
void asmPop (struct asmCtx* ctx, operand L);

void asmPushN (struct asmCtx* ctx, int n);
void asmPopN (struct asmCtx* ctx, int n);

void asmMove (struct asmCtx* ctx, operand Dest, operand Src);
void asmConditionalMove (struct asmCtx* ctx, operand Cond, operand Dest, operand Src);

void asmEvalAddress (struct asmCtx* ctx, operand L, operand R);

void asmCompare (struct asmCtx* ctx, operand L, operand R);

void asmBOP (struct asmCtx* ctx, boperation Op, operand L, operand R);
void asmUOP (struct asmCtx* ctx, uoperation Op, operand R);

/**
 * Call a function with the arguments currently on the stack
 */
void asmCall (struct asmCtx* ctx, operand L);
