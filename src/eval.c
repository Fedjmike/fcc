#include "../inc/eval.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/architecture.h"

static evalResult evalBOP (const architecture* arch, const ast* Node);
static evalResult evalUOP (const architecture* arch, const ast* Node);
static evalResult evalTernary (const architecture* arch, const ast* Node);
static evalResult evalCast (const architecture* arch, const ast* Node);
static evalResult evalSizeof (const architecture* arch, const ast* Node);
static evalResult evalLiteral (const architecture* arch, const ast* Node);

evalResult eval (const architecture* arch, const ast* Node) {
    if (Node->tag == astBOP)
        return evalBOP(arch, Node);

    else if (Node->tag == astUOP)
        return evalUOP(arch, Node);

    else if (Node->tag == astTOP)
        return evalTernary(arch, Node);

    else if (Node->tag == astCast)
        return evalCast(arch, Node);

    else if (Node->tag == astSizeof)
        return evalSizeof(arch, Node);

    else if (Node->tag == astLiteral)
        return evalLiteral(arch, Node);

    /*Never known*/
    else if (   Node->tag == astCall || Node->tag == astIndex
             || Node->tag == astVAStart || Node->tag == astVAEnd
             || Node->tag == astVAArg|| Node->tag == astVACopy)
        return (evalResult) {false, 0};

    else if (Node->tag == astInvalid)
        return (evalResult) {false, 0};

    else {
        debugErrorUnhandled("eval", "AST tag", astTagGetStr(Node->tag));
        return (evalResult) {false, 0};
    }
}

static evalResult evalBOP (const architecture* arch, const ast* Node) {
    evalResult L = eval(arch, Node->l),
               R = eval(arch, Node->r);

    if (opIsAssignment(Node->o))
        return (evalResult) {false, 0};

    else if (Node->o == opComma)
        return (evalResult) {L.known && R.known, R.value};

    else if (opIsMember(Node->o))
        return R;

    else if (Node->o == opLogicalAnd) {
        /*Both known*/
        if (L.known && R.known)
            return (evalResult) {true, L.value && R.value};

        /*One known and false*/
        else if ((L.known && !L.value) || (R.known && !R.value))
            return (evalResult) {true, false};

        else
            return (evalResult) {false, 0};

    } else if (Node->o == opLogicalOr) {
        /*Both known*/
        if (L.known && R.known)
            return (evalResult) {true, L.value && R.value};

        /*One known and true*/
        else if ((L.known && L.value) || (R.known && R.value))
            return (evalResult) {true, true};

        else
            return (evalResult) {false, 0};

    } else {
        int result;

        if (Node->o == opBitwiseAnd) result = L.value & R.value;
        else if (Node->o == opBitwiseOr) result = L.value | R.value;
        else if (Node->o == opBitwiseXor) result = L.value ^ R.value;
        else if (Node->o == opEqual) result = (int)(L.value == R.value);
        else if (Node->o == opNotEqual) result = (int)(L.value != R.value);
        else if (Node->o == opGreater) result = (int)(L.value > R.value);
        else if (Node->o == opGreaterEqual) result = (int)(L.value >= R.value);
        else if (Node->o == opLess) result = (int)(L.value < R.value);
        else if (Node->o == opLessEqual) result = (int)(L.value <= R.value);
        else if (Node->o == opShr) result = L.value >> R.value;
        else if (Node->o == opShl) result = L.value << R.value;
        else if (Node->o == opAdd) result = L.value + R.value;
        else if (Node->o == opSubtract) result = L.value - R.value;
        else if (Node->o == opMultiply) result = L.value * R.value;
        else if (Node->o == opDivide) result = L.value / R.value;
        else if (Node->o == opModulo) result = L.value % R.value;
        else {
            debugErrorUnhandled("evalBOP", "operator", opTagGetStr(Node->o));
            return (evalResult) {false, 0};
        }

        return (evalResult) {L.known && R.known, result};
    }
}

static evalResult evalUOP (const architecture* arch, const ast* Node) {
    if (   Node->o == opAddressOf || Node->o == opDeref
        || Node->o == opPostIncrement || Node->o == opPostDecrement
        || Node->o == opPreIncrement || Node->o == opPreDecrement)
        return (evalResult) {false, 0};

    else {
        evalResult R = eval(arch, Node->r);
        int result;

        if (Node->o == opLogicalNot) result = (int) !R.value;
        else if (Node->o == opBitwiseNot) result = ~R.value;
        else if (Node->o == opUnaryPlus) result = R.value;
        else if (Node->o == opNegate) result = -R.value;
        else {
            debugErrorUnhandled("evalUOP", "operator", opTagGetStr(Node->o));
            return (evalResult) {false, 0};
        }

        return (evalResult) {R.known, result};
    }
}

static evalResult evalTernary (const architecture* arch, const ast* Node) {
    evalResult Cond = eval(arch, Node->firstChild),
               L = eval(arch, Node->l),
               R = eval(arch, Node->r);

    /*Condition and the necessary operand known*/
    if (Cond.known)
        return (evalResult) {Cond.value ? L.known : R.known,
                             Cond.value ? L.value : R.value};

    /*Both operands, and they're equal*/
    else if (L.known && R.known && L.value == R.value)
        return (evalResult) {true, L.value};

    /*Unknown*/
    else
        return (evalResult) {false, 0};
}

static evalResult evalCast (const architecture* arch, const ast* Node) {
    return eval(arch, Node->r);
}

static evalResult evalSizeof (const architecture* arch, const ast* Node) {
    return (evalResult) {true, typeGetSize(arch, Node->dt)};
}

static evalResult evalLiteral (const architecture* arch, const ast* Node) {
    (void) arch;

    if (Node->litTag == literalInt)
        return (evalResult) {true, *(int*) Node->literal};

    else if (Node->litTag == literalChar)
        return (evalResult) {true, *(char*) Node->literal};

    else if (Node->litTag == literalBool)
        return (evalResult) {true, *(char*) Node->literal};

    /*Only enum constants are known at compile time*/
    else if (Node->litTag == literalIdent)
        return (evalResult) {Node->symbol && Node->symbol->tag == symEnumConstant,
                             Node->symbol ? Node->symbol->constValue : 0};

    else if (   Node->litTag == literalStr
             || Node->litTag == literalCompound
             || Node->litTag == literalInit
             || Node->litTag == literalLambda)
        return (evalResult) {false, 0};

    else {
        debugErrorUnhandled("evalLiteral", "literal tag", opTagGetStr(Node->o));
        return (evalResult) {false, 0};
    }
}

bool evalIsConstantInit (const ast* Node) {
    /*Compound initializer: constant if all the fields/elements are constant*/
    if (Node->tag == astLiteral && Node->litTag == literalInit) {
        for (ast* current = Node->firstChild;
             current;
             current = current->nextSibling) {
            ast* value = current;

            /*Designated initializer?*/
            if (   current->tag == astMarker
                && (   current->marker == markerStructDesignatedInit
                    || current->marker == markerArrayDesignatedInit))
                value = current->r;

            if (!evalIsConstantInit(value))
                return false;
        }

        return true;

    } else
        return eval(&(architecture) {}, Node).known;
}
