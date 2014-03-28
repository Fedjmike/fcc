#pragma once

#include "../std/std.h"

#include "parser.h"

struct type;
struct sym;

typedef enum {
    astUndefined,
    astInvalid,
    astEmpty,
    astModule,
    astUsing,
    astFnImpl,
    astType, astDecl, astParam, astStruct, astUnion, astEnum, astConst,
    astCode, astBranch, astLoop, astIter, astReturn, astBreak, astContinue,
    astBOP, astUOP, astTOP, astIndex, astCall, astCast, astSizeof, astLiteral,
    astEllipsis
} astTag;

typedef enum opTag {
    opUndefined,
    opComma,
    opAssign,
    opBitwiseAndAssign, opBitwiseOrAssign, opBitwiseXorAssign,
    opTernary,
    opShrAssign, opShlAssign,
    opAddAssign, opSubtractAssign, opMultiplyAssign, opDivideAssign, opModuloAssign,
    opLogicalAnd, opLogicalOr, opBitwiseAnd, opBitwiseOr, opBitwiseXor,
    opEqual, opNotEqual, opGreater, opGreaterEqual, opLess, opLessEqual,
    opShr, opShl,
    opAdd, opSubtract, opMultiply, opDivide, opModulo,
    opLogicalNot, opBitwiseNot, opUnaryPlus, opNegate, opDeref, opAddressOf,
    opIndex,
    opPreIncrement, opPreDecrement, opPostIncrement, opPostDecrement,
    opMember, opMemberDeref
} opTag;

typedef enum {
    literalUndefined,
    literalIdent,
    literalInt,
    literalChar,
    literalBool,
    literalStr,
    literalCompound,
    literalInit
} literalTag;

typedef struct ast {
    astTag tag;

    tokenLocation location;

    /*Linked list for container nodes like a module or parameter list and children of containers*/
    struct ast* firstChild;
    struct ast* lastChild;
    struct ast* nextSibling;
    struct ast* prevSibling;
    int children;

    /*Binary tree*/
    struct ast* l;
    char* o;
    struct ast* r;      /*Always used for unary operators*/
    struct type* dt;    /*Result data type*/

    struct sym* symbol;

    /*Literals*/
    literalTag litTag;
    void* literal;
} ast;

ast* astCreate (astTag tag, tokenLocation location);
void astDestroy (ast* Node);

ast* astCreateInvalid (tokenLocation location);
ast* astCreateEmpty (tokenLocation location);

ast* astCreateUsing (tokenLocation location, char* name);

ast* astCreateFnImpl (tokenLocation location, ast* decl);

ast* astCreateType (tokenLocation location, ast* basic, ast* expr);
ast* astCreateDecl (tokenLocation location, ast* basic);
ast* astCreateParam (tokenLocation location, ast* basic, ast* expr);
ast* astCreateStruct (tokenLocation location, ast* name);
ast* astCreateUnion (tokenLocation location, ast* name);
ast* astCreateEnum (tokenLocation location, ast* name);
ast* astCreateConst (tokenLocation location, ast* r);

ast* astCreateBOP (tokenLocation location, ast* l, char* o, ast* r);
ast* astCreateUOP (tokenLocation location, char* o, ast* r);
ast* astCreateTOP (tokenLocation location, ast* cond, ast* l, ast* r);
ast* astCreateIndex (tokenLocation location, ast* base, ast* index);
ast* astCreateCall (tokenLocation location, ast* function);
ast* astCreateCast (tokenLocation location, ast* result, ast* r);
ast* astCreateSizeof (tokenLocation location, ast* r);
ast* astCreateLiteral (tokenLocation location, literalTag litTag);
ast* astCreateLiteralIdent (tokenLocation location, char* ident);

ast* astCreateEllipsis (tokenLocation location);

void astAddChild (ast* Parent, ast* Child);

bool astIsValueTag (astTag tag);

/**
 * Return the string associated with an AST tag
 *
 * Does not allocate a new string, so no need to free it.
 */
const char* astTagGetStr (astTag tag);

const char* literalTagGetStr (literalTag tag);

/**
 * Returns whether the (binary) operator is one that can only act on
 * numeric types (e.g. int, char, not bool, not x*)
 */
bool opIsNumeric (opTag o);

bool opIsBitwise (opTag o);

/**
 * Is it an ordinal operator (defines an ordering...)?
 */
bool opIsOrdinal (opTag o);

bool opIsEquality (opTag o);
bool opIsAssignment (opTag o);
bool opIsLogical (opTag o);

/**
 * Does this operator access struct/union members of its LHS?
 */
bool opIsMember (opTag o);

/**
 * Does it dereference its LHS?
 */
bool opIsDeref (opTag o);

const char* opTagGetStr (opTag tag);
