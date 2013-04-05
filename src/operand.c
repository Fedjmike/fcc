#include "../inc/operand.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/reg.h"

char* Conditions[] = {"condition", "e", "ne", "g", "ge", "l", "le"};

char* labels[1024] = {"undefined"};
int labelNo = 1;

operand operandCreate (operandClass class) {
    operand ret;
    ret.class = class;
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

operand operandCreateLabelOffset (operand label) {
    operand ret = operandCreate(operandLabelOffset);
    ret.label = label.label;

    return ret;
}

int operandGetSize (operand Value) {
    if (Value.class == operandReg)
        return 8;

    else if (Value.class == operandMem || Value.class == operandMemRef)
        return Value.size;

    else if (Value.class == operandLiteral)
        return 1;

    else if (Value.class == operandLabelOffset)
        return 8;

    else
        debugErrorUnhandled("operandGetSize", "operand class", operandClassGetStr(Value.class));

    return 0;
}

char* operandToStr (operand Value) {
    char* ret = malloc(32);
    ret[0] = 0;

    if (Value.class == operandFlags)
        strncpy(ret, Conditions[Value.condition], 32);

    else if (Value.class == operandReg)
        strncpy(ret, regToStr(Value.reg), 32);

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
        snprintf(ret, 32, "%d", Value.literal);

    else if (Value.class == operandLabel)
        strncpy(ret, labelGet(Value), 32);

    else if (Value.class == operandLabelOffset)
        snprintf(ret, 32, "offset %s", labelGet(Value));

    else
        debugErrorUnhandled("operandToStr", "operand class", operandClassGetStr(Value.class));

    return ret;
}

void operandFree (operand Value) {
    if (Value.class == operandReg)
        regFree(Value.reg);

    else if (Value.class == operandMem || Value.class == operandMemRef) {
        if (Value.reg != regUndefined)
            regFree(Value.reg);

        if (Value.index != regUndefined)
            regFree(Value.index);

    } else if (   Value.class == operandUndefined || Value.class == operandInvalid
               || Value.class == operandFlags || Value.class == operandLiteral
               || Value.class == operandLabel || Value.class == operandLabelOffset
               || Value.class == operandStack)
        /*Nothing to do*/;

    else
        debugErrorUnhandled("operandFree", "operand class", operandClassGetStr(Value.class));
}

const char* operandClassGetStr (operandClass class) {
    if (class == operandUndefined)
        return "operandUndefined";
    else if (class == operandFlags)
        return "operandFlags";
    else if (class == operandReg)
        return "operandReg";
    else if (class == operandMem)
        return "operandMem";
    else if (class == operandMemRef)
        return "operandMemRef";
    else if (class == operandLiteral)
        return "operandLiteral";
    else if (class == operandLabel)
        return "operandLabel";
    else if (class == operandLabelOffset)
        return "operandLabelOffset";
    else if (class == operandStack)
        return "operandStack";

    else {
        char* str = malloc(logi(class, 10)+2);
        sprintf(str, "%d", class);
        debugErrorUnhandled("operandClassGetStr", "symbol class", str);
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

operand labelCreate (int class) {
    char* label = malloc(12);

    if (class == labelReturn)
        sprintf(label, "ret%08d", labelNo);

    else if (class == labelBreak)
        sprintf(label, "brk%08d", labelNo);

    else if (class == labelROData)
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
    if (Label.class == operandLabel || Label.class == operandLabelOffset)
        return labels[Label.label];

    else {
        printf("labelGet(): expected label operand, got %d.\n", Label.class);
        return 0;
    }
}

void labelFreeAll () {
    for (int i = 1; i < labelNo; i++)
        free(labels[i]);

    labelNo = 1;
}
