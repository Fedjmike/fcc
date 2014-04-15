#include "../inc/operand.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "../inc/debug.h"
#include "../inc/ast.h"
#include "../inc/architecture.h"
#include "../inc/reg.h"

using "../inc/operand.h";

using "stdlib.h";
using "string.h";
using "stdio.h";

using "../inc/debug.h";
using "../inc/ast.h";
using "../inc/architecture.h";
using "../inc/reg.h";

operand operandCreate (operandTag tag) {
    operand ret;
    ret.tag = tag;
    ret.base = regUndefined;
    ret.index = regUndefined;
    ret.factor = 0;
    ret.offset = 0;
    ret.literal = 0;
    ret.condition = conditionUndefined;
    ret.label = 0;
    ret.array = false;
    ret.size = 0;
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

operand operandCreateLiteral (int literal) {
    operand ret = operandCreate(operandLiteral);
    ret.literal = literal;
    return ret;
}

operand operandCreateLabel (const char* label) {
    operand ret = operandCreate(operandLabel);
    ret.label = label;
    return ret;
}

operand operandCreateLabelMem (const char* label, int size) {
    operand ret = operandCreate(operandLabelMem);
    ret.label = label;
    ret.size = size;
    return ret;
}

operand operandCreateLabelOffset (const char* label) {
    operand ret = operandCreate(operandLabelOffset);
    ret.label = label;
    return ret;
}

void operandFree (operand Value) {
    if (Value.tag == operandReg)
        regFree(Value.base);

    else if (Value.tag == operandMem) {
        if (Value.base != 0 && Value.base != regGet(regRBP))
            regFree(Value.base);

        if (Value.index != 0)
            regFree(Value.index);

    } else if (   Value.tag == operandUndefined || Value.tag == operandInvalid
               || Value.tag == operandVoid || Value.tag == operandFlags
               || Value.tag == operandLiteral || Value.tag == operandLabel
               || Value.tag == operandLabelMem || Value.tag == operandLabelOffset
               || Value.tag == operandStack)
        /*Nothing to do*/;

    else
        debugErrorUnhandled("operandFree", "operand tag", operandTagGetStr(Value.tag));
}

bool operandIsEqual (operand L, operand R) {
    if (L.tag != R.tag)
        return false;

    else if (L.tag == operandFlags)
        return L.condition == R.condition;

    else if (L.tag == operandReg)
        return L.base == R.base;

    else if (L.tag == operandMem)
        return    L.size == R.size && L.base == R.base
               && L.index == R.index && L.factor == R.factor
               && L.offset == R.offset;

    else if (L.tag == operandLiteral)
        return L.literal == R.literal;

    else if (L.tag == operandLabel || L.tag == operandLabelMem || L.tag == operandLabelOffset)
        return L.label == R.label;

    else /*if (   L.tag == operandVoid || L.tag == operandStack
               || L.tag == operandInvalid || L.tag == operandUndefined)*/
        return true;
}

int operandGetSize (const architecture* arch, operand Value) {
    if (   Value.tag == operandUndefined
        || Value.tag == operandInvalid || Value.tag == operandVoid)
        return 0;

    else if (Value.tag == operandReg)
        return Value.base->allocatedAs;

    else if (   Value.tag == operandMem || Value.tag == operandLabelMem)
        return Value.size;

    else if (Value.tag == operandLiteral)
        return 1;

    else if (   Value.tag == operandLabel || Value.tag == operandLabelOffset
             || Value.tag == operandFlags)
        return arch->wordsize;

    else
        debugErrorUnhandled("operandGetSize", "operand tag", operandTagGetStr(Value.tag));

    return 0;
}

char* operandToStr (operand Value) {
    if (Value.tag == operandUndefined)
        return strdup("<undefined>");

    else if (Value.tag == operandInvalid)
        return strdup("<invalid>");

    else if (Value.tag == operandVoid)
        return strdup("<void>");

    else if (Value.tag == operandFlags) {
        const char* conditions[7] = {"condition", "e", "ne", "g", "ge", "l", "le"};
        return strdup(conditions[Value.condition]);

    } else if (Value.tag == operandReg)
        return strdup(regGetStr(Value.base));

    else if (   Value.tag == operandMem || Value.tag == operandLabelMem) {
        const char* sizeStr;

        if (Value.size == 1)
            sizeStr = "byte";
        else if (Value.size == 2)
            sizeStr = "word";
        else if (Value.size == 4)
            sizeStr = "dword";
        else if (Value.size == 8)
            sizeStr = "qword";
        else if (Value.size == 16)
            sizeStr = "oword";
        else
            sizeStr = "undefined";

        if (Value.tag == operandLabelMem) {
            char* ret = malloc(  strlen(sizeStr)
                               + strlen(Value.label) + 9);
            sprintf(ret, "%s ptr [%s]", sizeStr, Value.label);
            return ret;

        } else if (Value.index == regUndefined || Value.factor == 0) {
            if (Value.offset == 0) {
                const char* regStr = regGetStr(Value.base);
                char* ret = malloc(  strlen(sizeStr)
                                   + strlen(regStr) + 9);
                sprintf(ret, "%s ptr [%s]", sizeStr, regStr);
                return ret;

            } else {
                const char* regStr = regGetStr(Value.base);
                char* ret = malloc(  strlen(sizeStr)
                                   + strlen(regStr)
                                   + logi(Value.offset, 10) + 2 + 9);
                sprintf(ret, "%s ptr [%s%+d]", sizeStr, regStr, Value.offset);
                return ret;
            }

        } else {
            const char* regStr = regGetStr(Value.base);
            const char* indexStr = regGetStr(Value.index);
            char* ret = malloc(  strlen(sizeStr)
                               + strlen(regStr)
                               + logi(Value.factor, 10) + 3
                               + strlen(indexStr)
                               + logi(Value.offset, 10) + 2 + 9);
            sprintf(ret, "%s ptr [%s%+d*%s%+d]",
                     sizeStr, regStr, Value.factor, indexStr, Value.offset);
            return ret;
        }

    } else if (Value.tag == operandLiteral) {
        char* ret = malloc(logi(Value.literal, 10)+3);
        sprintf(ret, "%d", Value.literal);
        return ret;

    } else if (Value.tag == operandLabel || Value.tag == operandLabelOffset) {
        char* ret = malloc(strlen(Value.label)+8);
        sprintf(ret, "offset %s", Value.label);
        return ret;

    } else {
        debugErrorUnhandled("operandToStr", "operand tag", operandTagGetStr(Value.tag));
        return strdup("<unhandled>");
    }
}

const char* operandTagGetStr (operandTag tag) {
    if (tag == operandUndefined) return "operandUndefined";
    else if (tag == operandInvalid) return "operandInvalid";
    else if (tag == operandVoid) return "operandVoid";
    else if (tag == operandFlags) return "operandFlags";
    else if (tag == operandReg) return "operandReg";
    else if (tag == operandMem) return "operandMem";
    else if (tag == operandLiteral) return "operandLiteral";
    else if (tag == operandLabel) return "operandLabel";
    else if (tag == operandLabelMem) return "operandLabelMem";
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

conditionTag conditionFromOp (opTag cond) {
    if (cond == opEqual) return conditionEqual;
    else if (cond == opNotEqual) return conditionNotEqual;
    else if (cond == opGreater) return conditionGreater;
    else if (cond == opGreaterEqual) return conditionGreaterEqual;
    else if (cond == opLess) return conditionLess;
    else if (cond == opLessEqual) return conditionLessEqual;
    else return conditionUndefined;
}

conditionTag conditionNegate (conditionTag cond) {
    if (cond == conditionEqual) return conditionNotEqual;
    else if (cond == conditionNotEqual) return conditionEqual;
    else if (cond == conditionGreater) return conditionLessEqual;
    else if (cond == conditionGreaterEqual) return conditionLess;
    else if (cond == conditionLess) return conditionGreaterEqual;
    else if (cond == conditionLessEqual) return conditionGreater;
    else return conditionUndefined;
}
