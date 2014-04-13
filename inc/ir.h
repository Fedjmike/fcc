typedef enum irInstrTag {
    instrUndefined,
    instrJump,
    instrBranch,
    instrMove,
    instrBOP,
    instrUOP
} irInstrTag;

typedef struct irInstr {
    irInstrTag tag;
    irOpTag opTag;

    operand *result, *l, *r;
} irInstr;

typedef struct irBlock {
    vector/*<irInstr*>*/ instrs;
} irBlock;

typedef struct irCtx {
    vector/*<irBlock*>*/ blocks;
} irCtx;

irBlock* irBlockCreate (irCtx* ir);
static void irBlockAdd (irBlock* block, irInstr* instr);

