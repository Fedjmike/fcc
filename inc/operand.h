#pragma once

#include "../std/std.h"

typedef struct reg reg;
typedef struct architecture architecture;
typedef enum opTag opTag;

typedef enum operandTag {
    operandUndefined,
    operandInvalid,
    operandVoid,
    operandFlags,
    operandReg,
    operandMem,
    operandLiteral,
    operandLabel,
    operandLabelMem,
    operandLabelOffset,
    operandStack
} operandTag;

typedef enum conditionTag {
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

    /*operandMem: size ptr [base + index*factor + offset]*/

    union {
        struct {
            /*operandReg operandMem*/
            reg* base;
            /*operandMem*/
            reg* index;
            int factor;
            int offset;
        };
        /*operandLiteral*/
        int literal;
        /*operandFlags*/
        ///Condition for the FALSE result
        conditionTag condition;
        /*operandLabel operandLabelMem operandLabelOffset*/
        const char* label;
    };

    ///In bytes, for mem operands
    int size;

    bool array;
} operand;

operand operandCreate (operandTag tag);
operand operandCreateInvalid (void);
operand operandCreateVoid (void);
operand operandCreateFlags (conditionTag cond);
operand operandCreateReg (reg* r);
operand operandCreateMem (reg* base, int offset, int size);
operand operandCreateLiteral (int literal);
operand operandCreateLabel (const char* label);
operand operandCreateLabelMem (const char* label, int size);
operand operandCreateLabelOffset (const char* label);

void operandFree (operand Value);

/**
 * Are the operands structurally, not semantically, equal
 */
bool operandIsEqual (operand L, operand R);

int operandGetSize (const architecture* arch, operand Value);

char* operandToStr (operand Value);

const char* operandTagGetStr (operandTag tag);

/* ::::CONDITIONS:::: */

conditionTag conditionFromOp (opTag cond);

conditionTag conditionNegate (conditionTag cond);
