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
} operandClass;

typedef enum {
    conditionUndefined,
    conditionEqual,
    conditionNotEqual,
    conditionGreater,
    conditionGreaterEqual,
    conditionLess,
    conditionLessEqual
} conditionClass;

typedef enum {
    labelUndefined,
    labelReturn,
    labelBreak,
    labelROData
} labelClass;

typedef struct {
    operandClass class;

    regClass reg;
    regClass index;
    int factor;
    int offset;
    int size; 		/*In bytes, for mem operands*/

    int literal;

    conditionClass condition;	/*Condition for the FALSE result*/

    int label; 		/*Global label index*/
} operand;

extern char* Conditions[];

operand operandCreate (operandClass class);
operand operandCreateInvalid ();
operand operandCreateFlags (conditionClass Condition);
operand operandCreateReg (regClass Reg);
operand operandCreateMem (int Reg, int Offset, int Size);
operand operandCreateMemRef (int Reg, int Offset, int Size);
operand operandCreateLiteral (int Literal);
operand operandCreateLabel (int Label);
operand operandCreateLabelOffset (operand label);

int operandGetSize (operand Value);

char* operandToStr (operand Value);

void operandFree (operand Value);

const char* operandClassGetStr (operandClass class);

/* ::::CONDITIONS:::: */

int conditionFromStr (char* Condition);

int conditionNegate (int Condition);

/* ::::LABELS:::: */

operand labelCreate (int class);

operand labelNamed (const char* Name);

const char* labelGet (operand Label);

void labelFreeAll ();
