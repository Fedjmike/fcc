#include "operand.h"

struct asmCtx;

typedef enum {
	bopUndefined,
	bopCmp,
	bopMul,
	bopAdd,
	bopSub,
} boperation;

typedef enum {
	uopUndefined,
	uopInc,
	uopDec
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
void asmFnPrologue (struct asmCtx* ctx, char* Name, int LocalSize);

/**
 * Emit the epilogue to a function
 */
void asmFnEpilogue (struct asmCtx* ctx, char* EndLabel);

/**
 * Place a previously named label in the output
 */
void asmLabel (struct asmCtx* ctx, operand L);

/**
 * Emit an unconditionally jump
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

void asmMove (struct asmCtx* ctx, operand L, operand R);
void asmEvalAddress (struct asmCtx* ctx, operand L, operand R);

/**
 * Perform a binary operation (e.g. add, mul, or)
 */
void asmBOP (struct asmCtx* ctx, boperation Op, operand L, operand R);

/**
 * Perform a unary operation (e.g. not, neg)
 */
void asmUOP (struct asmCtx* ctx, uoperation Op, operand L);

/**
 * Call a function with the arguments currently on the stack
 */
void asmCall (struct asmCtx* ctx, operand L);
