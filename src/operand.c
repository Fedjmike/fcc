#include "../inc/operand.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/reg.h"

char* Conditions[] = {"condition", "e", "ne", "g", "ge", "l", "le"};

char* labels[1024] = {"undefined"};
int labelNo = 1;

operand operandCreate (operandTag tag) {
    operand ret;
    ret.tag = tag;
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

operand operandCreateInvalid () {
    return operandCreate(operandInvalid);
}

operand operandCreateFlags (conditionTag Condition) {
    operand ret = operandCreate(operandFlags);
    ret.condition = Condition;

    return ret;
}

operand operandCreateReg (regTag Reg) {
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

operand operandCreateLabelOffset (operand label) {
    operand ret = operandCreate(operandLabelOffset);
    ret.label = label.label;

    return ret;
}

int operandGetSize (operand Value) {
    if (Value.tag == operandReg)
        return 8;

    else if (Value.tag == operandMem || Value.tag == operandMemRef)
        return Value.size;

    else if (Value.tag == operandLiteral)
        return 1;

    else if (Value.tag == operandLabelOffset)
        return 8;

    else
        debugErrorUnhandled("operandGetSize", "operand tag", operandTagGetStr(Value.tag));

    return 0;
}

char* operandToStr (operand Value) {
    char* ret = malloc(32);
    ret[0] = 0;

    if (Value.tag == operandFlags)
        strncpy(ret, Conditions[Value.condition], 32);

    else if (Value.tag == operandReg)
        strncpy(ret, regToStr(Value.reg), 32);

    else if (Value.tag == operandMem || Value.tag == operandMemRef) {
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

    } else if (Value.tag == operandLiteral)
        snprintf(ret, 32, "%d", Value.literal);

    else if (Value.tag == operandLabel)
        strncpy(ret, labelGet(Value), 32);

    else if (Value.tag == operandLabelOffset)
        snprintf(ret, 32, "offset %s", labelGet(Value));

    else
        debugErrorUnhandled("operandToStr", "operand tag", operandTagGetStr(Value.tag));

    return ret;
}

void operandFree (operand Value) {
    if (Value.tag == operandReg)
        regFree(Value.reg);

    else if (Value.tag == operandMem || Value.tag == operandMemRef) {
        if (Value.reg != regUndefined)
            regFree(Value.reg);

        if (Value.index != regUndefined)
            regFree(Value.index);

    } else if (   Value.tag == operandUndefined || Value.tag == operandInvalid
               || Value.tag == operandFlags || Value.tag == operandLiteral
               || Value.tag == operandLabel || Value.tag == operandLabelOffset
               || Value.tag == operandStack)
        /*Nothing to do*/;

    else
        debugErrorUnhandled("operandFree", "operand tag", operandTagGetStr(Value.tag));
}

const char* operandTagGetStr (operandTag tag) {
    if (tag == operandUndefined)
        return "operandUndefined";
    else if (tag == operandFlags)
        return "operandFlags";
    else if (tag == operandReg)
        return "operandReg";
    else if (tag == operandMem)
        return "operandMem";
    else if (tag == operandMemRef)
        return "operandMemRef";
    else if (tag == operandLiteral)
        return "operandLiteral";
    else if (tag == operandLabel)
        return "operandLabel";
    else if (tag == operandLabelOffset)
        return "operandLabelOffset";
    else if (tag == operandStack)
        return "operandStack";

    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("operandTagGetStr", "symbol tag", str);
        free(str);
        return "unhandled";
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

operand labelCreate (int tag) {
    char* label = malloc(12);

    if (tag == labelReturn)
        sprintf(label, "ret%08d", labelNo);

    else if (tag == labelBreak)
        sprintf(label, "brk%08d", labelNo);

    else if (tag == labelROData)
        sprintf(label, "rod%08d", labelNo);

    else
        sprintf(label, "_%08i", labelNo);

    labels[labelNo] = label;

    return operandCreateLabel(labelNo++);
}

operand labelNamed (const char* Name) {
    labels[labelNo] = malloc(strlen(Name)+2);
    sprintf(labels[labelNo], "_%s", Name);
    return operandCreateLabel(labelNo++);
}

const char* labelGet (operand Label) {
    if (Label.tag == operandLabel || Label.tag == operandLabelOffset)
        return labels[Label.label];

    else {
        printf("labelGet(): expected label operand, got %d.\n", Label.tag);
        return 0;
    }
}

void labelFreeAll () {
    for (int i = 1; i < labelNo; i++)
        free(labels[i]);

    labelNo = 1;
}
