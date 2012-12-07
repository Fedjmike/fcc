#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "../inc/operand.h"
#include "../inc/reg.h"

char* Conditions[] = {"condition", "e", "ne", "g", "ge", "l", "le"};

int labelUndefined = 0;
int labelReturn = 1;
int labelBreak = 2;

char* labels[1024] = {"undefined"};
int labelNo = 1;

operand operandCreate (operandClass Class) {
    operand ret;
    ret.class = Class;
    ret.reg = regUndefined;
    ret.index = regUndefined;
    ret.factor = 0;
    ret.offset = 0;
    ret.size = 0;
    ret.literal = 0;
    ret.condition = conditionUndefined;
    ret.label = 0;

    return ret;
}

operand operandCreateFlags (conditionClass Condition) {
    operand ret = operandCreate(operandFlags);
    ret.condition = Condition;

    return ret;
}

operand operandCreateReg (regClass Reg) {
    operand ret = operandCreate(operandReg);
    ret.reg = Reg;

    return ret;
}

operand operandCreateMem (int Reg, int Offset, int Size) {
    operand ret = operandCreate(operandMem);
    ret.reg = Reg;
    ret.index = regUndefined;
    ret.factor = 0;
    ret.offset = Offset;
    ret.size = Size;

    return ret;
}

operand operandCreateMemRef (int Reg, int Offset, int Size) {
    operand ret = operandCreate(operandMemRef);
    ret.reg = Reg;
    ret.index = regUndefined;
    ret.factor = 0;
    ret.offset = Offset;
    ret.size = Size;

    return ret;
}

operand operandCreateLiteral (int Literal) {
    operand ret = operandCreate(operandLiteral);
    ret.literal = Literal;

    return ret;
}

operand operandCreateLabel (int Label) {
    operand ret = operandCreate(operandLabel);
    ret.label = Label;

    return ret;
}

int operandGetSize (operand Value) {
    if (Value.class == operandReg)
        return 8;

    else if (Value.class == operandMem || Value.class == operandMemRef)
        return Value.size;

    else if (Value.class == operandLiteral)
        return 1;

    else
        printf("operandGetSize(): unhandled operand class, %d.\n", Value.class);

    return 0;
}

char* operandToStr (operand Value) {
    char* ret = malloc(32);
    ret[0] = 0;

    if (Value.class == operandFlags)
        strcat(ret, Conditions[Value.condition]);

    else if (Value.class == operandReg)
        strcat(ret, regToStr(Value.reg));

    else if (Value.class == operandMem || Value.class == operandMemRef) {
        char* Size;

        if (Value.size == 1)
            Size = "byte";

        else if (Value.size == 2)
            Size = "word";

        else if (Value.size == 4)
            Size = "dword";

        else if (Value.size == 8)
            Size = "qword";

        else if (Value.size == 16)
            Size = "oword";

        else
            Size = "undefined";

        if (Value.index == regUndefined || Value.factor == 0)
            if (Value.offset == 0)
                snprintf(ret, 32, "%s ptr [%s]", Size, regToStr(Value.reg));

            else
                snprintf(ret, 32, "%s ptr [%s%+d]", Size, regToStr(Value.reg), Value.offset);

        else
            snprintf(ret, 32, "%s ptr [%s%+d*%s%+d]", Size, regToStr(Value.reg),
                                                            Value.factor, regToStr(Value.index),
                                                            Value.offset);

    } else if (Value.class == operandLiteral)
        sprintf(ret, "%d", Value.literal);

    else if (Value.class == operandLabel)
        strcat(ret, labelGet(Value));

    else
        printf("operandToStr(): unknown operand class, %d.\n", Value.class);

    return ret;
}

void operandFree (operand Value) {
    if (Value.class == operandReg)
        regFree(Value.reg);

    if (Value.class == operandMem) {
        if (Value.reg != regUndefined)
            regFree(Value.reg);

        if (Value.index != regUndefined)
            regFree(Value.index);
    }
}

/* ::::CONDITIONS:::: */

int conditionFromStr (char* Condition) {
    if (!strcmp(Condition, "=="))
        return conditionEqual;
    else if (!strcmp(Condition, "!="))
        return conditionNotEqual;
    else if (!strcmp(Condition, ">"))
        return conditionGreater;
    else if (!strcmp(Condition, ">="))
        return conditionGreaterEqual;
    else if (!strcmp(Condition, "<"))
        return conditionLess;
    else if (!strcmp(Condition, "<="))
        return conditionLessEqual;
    else
        return conditionUndefined;
}

int conditionNegate (int Condition) {
    if (Condition == conditionEqual)
        return conditionNotEqual;
    else if (Condition == conditionNotEqual)
        return conditionEqual;
    else if (Condition == conditionGreater)
        return conditionLessEqual;
    else if (Condition == conditionGreaterEqual)
        return conditionLess;
    else if (Condition == conditionLess)
        return conditionGreaterEqual;
    else if (Condition == conditionLessEqual)
        return conditionGreater;
    else
        return conditionUndefined;
}

/* ::::LABELS:::: */

operand labelCreate (int Class) {
    static int Labels = 0;

    char* Label = malloc(12);

    if (Class == labelReturn)
        sprintf(Label, "ret%08d", Labels++);

    else if (Class == labelBreak)
        sprintf(Label, "brk%08d", Labels++);

    else
        sprintf(Label, "_%08i", Labels++);

    labels[labelNo] = Label;

    return operandCreateLabel(labelNo++);
}

operand labelNamed (char* Name) {
    labels[labelNo] = strcat(strcpy(malloc(strlen(Name)+2), "_"), Name);
    return operandCreateLabel(labelNo++);
}

char* labelGet (operand Label) {
    if (Label.class == operandLabel)
        return labels[Label.label];

    else {
        printf("labelGet(): expected label operand, got %d.\n", Label.class);
        return 0;
    }
}

void labelFreeAll () {
    for (int i = 0; i < labelNo; i++)
        free(labels[i]);

    labelNo = 0;
}
