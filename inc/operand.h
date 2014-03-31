#pragma once

struct reg;
struct architecture;
typedef enum opTag opTag;

typedef enum {
    operandUndefined,
    operandInvalid,
    operandVoid,
    operandFlags,
    operandReg,
    operandMem,
    operandMemRef,
    operandLiteral,
    operandLabel,
    operandLabelMem,
    operandLabelOffset,
    operandStack
} operandTag;

typedef enum {
    conditionUndefined,
    conditionEqual,
    conditionNotEqual,
    conditionGreater,
    conditionGreaterEqual,
    conditionLess,
    conditionLessEqual
} conditionTag;

typedef struct operand {
    operandTag tag;

    struct reg* base;
    struct reg* index;
    int factor;
    int offset;
    ///In bytes, for mem operands
    int size;

    int literal;

    ///Condition for the FALSE result
    conditionTag condition;

    const char* label;
} operand;

operand operandCreate (operandTag tag);
operand operandCreateInvalid (void);
operand operandCreateVoid (void);
operand operandCreateFlags (conditionTag cond);
operand operandCreateReg (struct reg* r);
operand operandCreateMem (struct reg* base, int offset, int size);
operand operandCreateMemRef (struct reg* base, int offset, int size);
operand operandCreateLiteral (int literal);
operand operandCreateLabel (const char* label);
operand operandCreateLabelMem (const char* label, int size);
operand operandCreateLabelOffset (const char* label);

void operandFree (operand Value);

/**
 * Are the operands structurally, not semantically, equal
 */
bool operandIsEqual (operand L, operand R);

int operandGetSize (const struct architecture* arch, operand Value);

char* operandToStr (operand Value);

const char* operandTagGetStr (operandTag tag);

/* ::::CONDITIONS:::: */

conditionTag conditionFromOp (opTag cond);

conditionTag conditionNegate (conditionTag cond);

/* ::::LABELS:::: */

operand labelCreate (labelTag tag);

operand labelNamed (const char* name);

const char* labelGet (operand label);

void labelFreeAll (void);
