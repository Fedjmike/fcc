#include "../inc/operand.h"

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

void asmFilePrologue ();
void asmFileEpilogue ();
void asmFnPrologue (char* Name, int LocalSize);
void asmFnEpilogue (char* EndLabel);

void asmLabel (operand L);
void asmJump (operand L);
void asmBranch (operand Condition, operand L);
void asmPush (operand L);
void asmPop (operand L);

/**
 * Remove n words from the stack and discard them
 */
void asmPopN (int n);

void asmMove (operand L, operand R);
void asmEvalAddress (operand L, operand R);

/**
 * Perform a binary operation (e.g. add, mul, or)
 */
void asmBOP (boperation Op, operand L, operand R);

void asmUOP (uoperation Op, operand L);

void asmCall (operand L);
