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
void asmFilePrologue (asmCtx* ctx);

/**
 * Emit the ending of a file
 */
void asmFileEpilogue (asmCtx* ctx);

/**
 * Emit the prologue to a function
 */
void asmFnPrologue (asmCtx* ctx, char* Name, int LocalSize);

/**
 * Emit the epilogue to a function
 */
void asmFnEpilogue (asmCtx* ctx, char* EndLabel);

/**
 * Place a previously named label in the output
 */
void asmLabel (asmCtx* ctx, operand L);

/**
 * Emit an unconditionally jump
 */
void asmJump (asmCtx* ctx, operand L);

/**
 * Emit a conditional jump
 */
void asmBranch (asmCtx* ctx, operand Condition, operand L);

/**
 * Pushe an operand onto the stack
 */
void asmPush (asmCtx* ctx, operand L);

/**
 * Pop a word off the stack, into an operand
 */
void asmPop (asmCtx* ctx, operand L);

/**
 * Remove n words from the stack and discard them
 */
void asmPopN (asmCtx* ctx, int n);

void asmMove (asmCtx* ctx, operand L, operand R);
void asmEvalAddress (asmCtx* ctx, operand L, operand R);

/**
 * Perform a binary operation (e.g. add, mul, or)
 */
void asmBOP (asmCtx* ctx, boperation Op, operand L, operand R);

/**
 * Perform a unary operation (e.g. not, neg)
 */
void asmUOP (asmCtx* ctx, uoperation Op, operand L);

/**
 * Call a function with the arguments currently on the stack
 */
void asmCall (asmCtx* ctx, operand L);
