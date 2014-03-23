#include "../inc/eval.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/architecture.h"

#include "string.h"

static evalResult evalBOP (const architecture* arch, ast* Node);
static evalResult evalUOP (const architecture* arch, ast* Node);
static evalResult evalTernary (const architecture* arch, ast* Node);
static evalResult evalCast (const architecture* arch, ast* Node);
static evalResult evalSizeof (const architecture* arch, ast* Node);
static evalResult evalLiteral (const architecture* arch, ast* Node);

evalResult eval (const architecture* arch, ast* Node) {
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
    else if (Node->tag == astCall || Node->tag == astIndex)
        return (evalResult) {false, 0};

    else if (Node->tag == astInvalid)
        return (evalResult) {false, 0};

    else {
        debugErrorUnhandled("eval", "AST tag", astTagGetStr(Node->tag));
        return (evalResult) {false, 0};
    }
}

static evalResult evalBOP (const architecture* arch, ast* Node) {
    evalResult L = eval(arch, Node->l),
               R = eval(arch, Node->r);

    if (   !strcmp(Node->o, "=")
        || !strcmp(Node->o, "+=") || !strcmp(Node->o, "-=")
        || !strcmp(Node->o, "*=") || !strcmp(Node->o, "/=")
        || !strcmp(Node->o, "%=")
        || !strcmp(Node->o, "&=") || !strcmp(Node->o, "|=")
        || !strcmp(Node->o, "^=")
        || !strcmp(Node->o, "<<=") || !strcmp(Node->o, ">>="))
        return (evalResult) {false, 0};

    else if (!strcmp(Node->o, ","))
        return (evalResult) {L.known && R.known, R.value};

    else if (!strcmp(Node->o, "->") || !strcmp(Node->o, "."))
        return R;

    else if (!strcmp(Node->o, "&&")) {
        /*Both known*/
        if (L.known && R.known)
            return (evalResult) {true, L.value && R.value};

        /*One known and false*/
        else if ((L.known && !L.value) || (R.known && !R.value))
            return (evalResult) {true, false};

        else
            return (evalResult) {false, 0};

    } else if (!strcmp(Node->o, "||")) {
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

        if (!strcmp(Node->o, "&")) result = L.value & R.value;
        else if (!strcmp(Node->o, "|")) result = L.value | R.value;
        else if (!strcmp(Node->o, "^")) result = L.value ^ R.value;
        else if (!strcmp(Node->o, "==")) result = L.value == R.value;
        else if (!strcmp(Node->o, "!=")) result = L.value != R.value;
        else if (!strcmp(Node->o, ">")) result = L.value > R.value;
        else if (!strcmp(Node->o, ">=")) result = L.value >= R.value;
        else if (!strcmp(Node->o, "<")) result = L.value < R.value;
        else if (!strcmp(Node->o, "<=")) result = L.value <= R.value;
        else if (!strcmp(Node->o, ">>")) result = L.value >> R.value;
        else if (!strcmp(Node->o, "<<")) result = L.value << R.value;
        else if (!strcmp(Node->o, "+")) result = L.value + R.value;
        else if (!strcmp(Node->o, "-")) result = L.value - R.value;
        else if (!strcmp(Node->o, "*")) result = L.value * R.value;
        else if (!strcmp(Node->o, "/")) result = L.value / R.value;
        else if (!strcmp(Node->o, "%")) result = L.value % R.value;
        else {
            debugErrorUnhandled("evalBOP", "operator", Node->o);
            return (evalResult) {false, 0};
        }

        return (evalResult) {L.known && R.known, result};
    }
}

static evalResult evalUOP (const architecture* arch, ast* Node) {
    if (   !strcmp(Node->o, "&") || !strcmp(Node->o, "*")
        || !strcmp(Node->o, "++") || !strcmp(Node->o, "--"))
        return (evalResult) {false, 0};

    else {
        evalResult R = eval(arch, Node->r);
        int result;

        if (!strcmp(Node->o, "!")) result = !R.value;
        else if (!strcmp(Node->o, "~")) result = ~R.value;
        else if (!strcmp(Node->o, "-")) result = -R.value;
        else {
            debugErrorUnhandled("evalUOP", "operator", Node->o);
            return (evalResult) {false, 0};
        }

        return (evalResult) {R.known, result};
    }
}

static evalResult evalTernary (const architecture* arch, ast* Node) {
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

static evalResult evalCast (const architecture* arch, ast* Node) {
    return eval(arch, Node->r);
}

static evalResult evalSizeof (const architecture* arch, ast* Node) {
    return (evalResult) {true, typeGetSize(arch, Node->dt)};
}

static evalResult evalLiteral (const architecture* arch, ast* Node) {
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
             || Node->litTag == literalInit)
        return (evalResult) {false, 0};

    else {
        debugErrorUnhandled("evalLiteral", "literal tag", Node->o);
        return (evalResult) {false, 0};
    }
}
