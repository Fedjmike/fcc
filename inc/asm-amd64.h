#include "operand.h"

struct asmCtx;

typedef enum {
	bopUndefined,
	bopCmp,
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
	uopNeg
} uoperation;

/**
 * Emit the opening of a file
 */
void asmFilePrologue (struct asmCtx* ctx);

/**
 * Emit the ending of a file
 */
void asmFileEpilogue (struct asmCtx* ctx);

/**
 * Emit the prologue to a function
 */
void asmFnPrologue (struct asmCtx* ctx, operand Name, int localSize);

/**
 * Emit the epilogue to a function
 */
void asmFnEpilogue (struct asmCtx* ctx, operand EndLabel);

/**
 * Place a string constant in the rodata section with the given label
 */
void asmStringConstant (struct asmCtx* ctx, operand label, char* str);

/**
 * Place a previously named label in the output
 */
void asmLabel (struct asmCtx* ctx, operand L);

/**
 * Emit an unconditional jump
 */
void asmJump (struct asmCtx* ctx, operand L);

/**
 * Emit a conditional jump
 */
void asmBranch (struct asmCtx* ctx, operand Condition, operand L);

/**
 * Pushe an operand onto the stack
 */
void asmPush (struct asmCtx* ctx, operand L);

/**
 * Pop a word off the stack, into an operand
 */
void asmPop (struct asmCtx* ctx, operand L);

/**
 * Remove n words from the stack and discard them
 */
void asmPopN (struct asmCtx* ctx, int n);

void asmMove (struct asmCtx* ctx, operand Dest, operand Src);

void asmConditionalMove (struct asmCtx* ctx, operand Cond, operand Dest, operand Src);

void asmEvalAddress (struct asmCtx* ctx, operand L, operand R);

/**
 * Perform a binary operation (e.g. add, mul, or)
 */
void asmBOP (struct asmCtx* ctx, boperation Op, operand L, operand R);

/**
 * Perform a unary operation (e.g. not, neg)
 */
void asmUOP (struct asmCtx* ctx, uoperation Op, operand R);

/**
 * Call a function with the arguments currently on the stack
 */
void asmCall (struct asmCtx* ctx, operand L);
