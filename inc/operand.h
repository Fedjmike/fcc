#pragma once

#include "reg.h"

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

    regTag reg;
    regTag index;
    int factor;
    int offset;
    int size; 		/*In bytes, for mem operands*/

    int literal;

    conditionTag condition;	/*Condition for the FALSE result*/

    int label; 		/*Global label index*/
} operand;

extern char* Conditions[];

operand operandCreate (operandTag tag);
operand operandCreateInvalid ();
operand operandCreateFlags (conditionTag Condition);
operand operandCreateReg (regTag Reg);
operand operandCreateMem (int Reg, int Offset, int Size);
operand operandCreateMemRef (int Reg, int Offset, int Size);
operand operandCreateLiteral (int Literal);
operand operandCreateLabel (int Label);
operand operandCreateLabelOffset (operand label);

int operandGetSize (operand Value);

char* operandToStr (operand Value);

void operandFree (operand Value);

const char* operandTagGetStr (operandTag tag);

/* ::::CONDITIONS:::: */

int conditionFromStr (char* Condition);

int conditionNegate (int Condition);

/* ::::LABELS:::: */

operand labelCreate (int tag);

operand labelNamed (const char* Name);

const char* labelGet (operand Label);

void labelFreeAll ();
