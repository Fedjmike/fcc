#include "operand.h"

typedef struct ast ast;
typedef struct irBlock irBlock;
typedef struct emitterCtx emitterCtx;
typedef enum regIndex regIndex;

operand emitterGetInReg (emitterCtx* ctx, irBlock* block, operand src, int size);

/**
 * Forcibly allocate a register, saving its old value on the stack if need be.
 * @see emitterGiveBackReg()
 */
operand emitterTakeReg (emitterCtx* ctx, irBlock* block, regIndex r, int* oldSize, int newSize);
void emitterGiveBackReg (emitterCtx* ctx, irBlock* block, regIndex r, int oldSize);

void emitterBranchOnValue (emitterCtx* ctx, irBlock* block, const ast* value,
                           irBlock* ifTrue, irBlock* ifFalse);

operand emitterWiden (emitterCtx* ctx, irBlock* block, operand R, int size);
operand emitterNarrow (emitterCtx* ctx, irBlock* block, operand R, int size);
