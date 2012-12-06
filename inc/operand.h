#pragma once

#include "reg.h"

typedef enum {
    operandUndefined,
    operandFlags,
    operandReg,
    operandMem,
    operandMemRef,

    operandLiteral,
    operandLabel,
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

typedef struct {
    operandClass class;

    int reg;
    int index;
    int factor;
    int offset;
    int size; 		/*In bytes, for mem operands*/

    int literal;

    conditionClass condition;	/*Condition for the FALSE result*/

    int label; 		/*Global label index*/
} operand;

extern char* Conditions[];

operand operandCreate (operandClass Class);
operand operandCreateFlags (conditionClass Condition);
operand operandCreateReg (regClass Reg);
operand operandCreateMem (int Reg, int Offset, int Size);
operand operandCreateMemRef (int Reg, int Offset, int Size);
operand operandCreateLiteral (int Literal);
operand operandCreateLabel (int Label);

int operandGetSize (operand Value);

char* operandToStr (operand Value);

void operandFree (operand Value);

/* ::::CONDITIONS:::: */

int conditionFromStr (char* Condition);

int conditionNegate (int Condition);

/* ::::LABELS:::: */

extern int labelUndefined;
extern int labelReturn;
extern int labelBreak;

operand labelCreate (int Class);

operand labelNamed (char* Name);

char* labelGet (operand Label);

void labelFreeAll ();
