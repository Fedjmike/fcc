#include "../inc/operand.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/reg.h"

const char *const conditions[] = {"condition", "e", "ne", "g", "ge", "l", "le"};

char* labels[1024] = {"undefined"};
int labelNo = 1;

operand operandCreate (operandTag tag) {
    operand ret;
    ret.tag = tag;
    ret.base = regUndefined;
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

operand operandCreateVoid () {
    return operandCreate(operandVoid);
}

operand operandCreateFlags (conditionTag cond) {
    operand ret = operandCreate(operandFlags);
    ret.condition = cond;
    return ret;
}

operand operandCreateReg (reg* r) {
    operand ret = operandCreate(operandReg);
    ret.base = r;
    return ret;
}

operand operandCreateMem (reg* base, int offset, int size) {
    operand ret = operandCreate(operandMem);
    ret.base = base;
    ret.index = regUndefined;
    ret.factor = 0;
    ret.offset = offset;
    ret.size = size;
    return ret;
}

operand operandCreateMemRef (reg* base, int offset, int size) {
    operand ret = operandCreate(operandMemRef);
    ret.base = base;
    ret.index = regUndefined;
    ret.factor = 0;
    ret.offset = offset;
    ret.size = size;
    return ret;
}

operand operandCreateLiteral (int literal) {
    operand ret = operandCreate(operandLiteral);
    ret.literal = literal;
    return ret;
}

operand operandCreateLabel (int label) {
    operand ret = operandCreate(operandLabel);
    ret.label = label;
    return ret;
}

operand operandCreateLabelOffset (operand label) {
    operand ret = operandCreate(operandLabelOffset);
    ret.label = label.label;
    return ret;
}

void operandFree (operand Value) {
    if (Value.tag == operandReg)
        regFree(Value.base);

    else if (Value.tag == operandMem || Value.tag == operandMemRef) {
        if (Value.base != 0 && Value.base != &regs[regRBP])
            regFree(Value.base);

        if (Value.index != 0)
            regFree(Value.index);

    } else if (   Value.tag == operandUndefined || Value.tag == operandInvalid
               || Value.tag == operandVoid || Value.tag == operandFlags
               || Value.tag == operandLiteral || Value.tag == operandLabel
               || Value.tag == operandLabelOffset || Value.tag == operandStack)
        /*Nothing to do*/;

    else
        debugErrorUnhandled("operandFree", "operand tag", operandTagGetStr(Value.tag));
}

int operandGetSize (const architecture* arch, operand Value) {
    if (   Value.tag == operandUndefined
        || Value.tag == operandInvalid || Value.tag == operandVoid)
        return 0;

    else if (Value.tag == operandReg)
        return Value.base->allocatedAs;

    else if (Value.tag == operandMem || Value.tag == operandMemRef)
        return Value.size;

    else if (Value.tag == operandLiteral)
        return 1;

    else if (Value.tag == operandLabelOffset)
        return arch->wordsize;

    else
        debugErrorUnhandled("operandGetSize", "operand tag", operandTagGetStr(Value.tag));

    return 0;
}

char* operandToStr (operand Value) {
    char* ret = malloc(32);
    ret[0] = 0;

    if (Value.tag == operandUndefined)
        return strcpy(ret, "<undefined>");

    else if (Value.tag == operandInvalid)
        return strcpy(ret, "<invalid>");

    else if (Value.tag == operandVoid)
        return strcpy(ret, "<void>");

    else if (Value.tag == operandFlags)
        strncpy(ret, conditions[Value.condition], 32);

    else if (Value.tag == operandReg)
        strncpy(ret, regToStr(Value.base), 32);

    else if (Value.tag == operandMem || Value.tag == operandMemRef) {
        const char* sizestr;

        if (Value.size == 1)
            sizestr = "byte";
        else if (Value.size == 2)
            sizestr = "word";
        else if (Value.size == 4)
            sizestr = "dword";
        else if (Value.size == 8)
            sizestr = "qword";
        else if (Value.size == 16)
            sizestr = "oword";
        else
            sizestr = "undefined";

        if (Value.index == regUndefined || Value.factor == 0)
            if (Value.offset == 0)
                snprintf(ret, 32, "%s ptr [%s]", sizestr, regToStr(Value.base));

            else
                snprintf(ret, 32, "%s ptr [%s%+d]", sizestr, regToStr(Value.base), Value.offset);

        else
            snprintf(ret, 32, "%s ptr [%s%+d*%s%+d]", sizestr, regToStr(Value.base),
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

const char* operandTagGetStr (operandTag tag) {
    if (tag == operandUndefined) return "operandUndefined";
    else if (tag == operandInvalid) return "operandInvalid";
    else if (tag == operandVoid) return "operandVoid";
    else if (tag == operandFlags) return "operandFlags";
    else if (tag == operandReg) return "operandReg";
    else if (tag == operandMem) return "operandMem";
    else if (tag == operandMemRef) return "operandMemRef";
    else if (tag == operandLiteral) return "operandLiteral";
    else if (tag == operandLabel) return "operandLabel";
    else if (tag == operandLabelOffset) return "operandLabelOffset";
    else if (tag == operandStack) return "operandStack";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("operandTagGetStr", "operand tag", str);
        free(str);
        return "<unhandled>";
    }
}

/* ::::CONDITIONS:::: */

int conditionFromStr (char* cond) {
    if (!strcmp(cond, "==")) return conditionEqual;
    else if (!strcmp(cond, "!=")) return conditionNotEqual;
    else if (!strcmp(cond, ">")) return conditionGreater;
    else if (!strcmp(cond, ">=")) return conditionGreaterEqual;
    else if (!strcmp(cond, "<")) return conditionLess;
    else if (!strcmp(cond, "<=")) return conditionLessEqual;
    else return conditionUndefined;
}

int conditionNegate (int cond) {
    if (cond == conditionEqual) return conditionNotEqual;
    else if (cond == conditionNotEqual) return conditionEqual;
    else if (cond == conditionGreater) return conditionLessEqual;
    else if (cond == conditionGreaterEqual) return conditionLess;
    else if (cond == conditionLess) return conditionGreaterEqual;
    else if (cond == conditionLessEqual) return conditionGreater;
    else return conditionUndefined;
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

operand labelNamed (const char* name) {
    labels[labelNo] = malloc(strlen(name)+2);
    sprintf(labels[labelNo], "_%s", name);
    return operandCreateLabel(labelNo++);
}

const char* labelGet (operand label) {
    if (label.tag == operandLabel || label.tag == operandLabelOffset)
        return labels[label.label];

    else {
        printf("labelGet(): expected label operand, got %d.\n", label.tag);
        return 0;
    }
}

void labelFreeAll () {
    for (int i = 1; i < labelNo; i++)
        free(labels[i]);

    labelNo = 1;
}
