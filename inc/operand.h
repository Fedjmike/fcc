#pragma once

struct reg;

typedef enum {
    operandUndefined,
    operandInvalid,
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

typedef struct {
    operandTag tag;

    struct reg* reg;
    struct reg* index;
    int factor;
    int offset;
    int size; 		/*In bytes, for mem operands*/

    int literal;

    conditionTag condition;	/*Condition for the FALSE result*/

    int label; 		/*Global label index*/
} operand;

extern const char const* conditions[];

operand operandCreate (operandTag tag);
operand operandCreateInvalid ();
operand operandCreateFlags (conditionTag cond);
operand operandCreateReg (struct reg* r);
operand operandCreateMem (struct reg* r, int offset, int size);
operand operandCreateMemRef (struct reg* r, int offset, int size);
operand operandCreateLiteral (int literal);
operand operandCreateLabel (int label);
operand operandCreateLabelOffset (operand label);

void operandFree (operand Value);

int operandGetSize (operand Value);

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
