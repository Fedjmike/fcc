#pragma once

struct reg;
struct architecture;

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

typedef enum {
    labelUndefined,
    labelReturn,
    labelBreak,
    labelROData
} labelTag;

typedef struct operand {
    operandTag tag;

    struct reg* base;
    struct reg* index;
    int factor;
    int offset;
    int size; 		/*In bytes, for mem operands*/

    int literal;

    conditionTag condition;	/*Condition for the FALSE result*/

    int label; 		/*Global label index*/
} operand;

extern const char *const conditions[];

operand operandCreate (operandTag tag);
operand operandCreateInvalid ();
operand operandCreateVoid ();
operand operandCreateFlags (conditionTag cond);
operand operandCreateReg (struct reg* r);
operand operandCreateMem (struct reg* base, int offset, int size);
operand operandCreateMemRef (struct reg* base, int offset, int size);
operand operandCreateLiteral (int literal);
operand operandCreateLabel (int label);
operand operandCreateLabelOffset (operand label);

void operandFree (operand Value);

int operandGetSize (const struct architecture* arch, operand Value);

char* operandToStr (operand Value);

const char* operandTagGetStr (operandTag tag);

/* ::::CONDITIONS:::: */

int conditionFromStr (char* cond);

int conditionNegate (int cond);

/* ::::LABELS:::: */

operand labelCreate (int tag);

operand labelNamed (const char* name);

const char* labelGet (operand label);

void labelFreeAll ();
