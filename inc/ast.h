#pragma once

#include "../std/std.h"

#include "sym.h"

#include "parser.h"

#include "stdint.h"

using "forward.h";

using "parser.h";

typedef struct type type;
typedef struct ast ast;

/**
 * Kind of AST node. Adding a new one requires:
 *  - Deciding if it is a value tag
 *  - Classifying it with astIsValueTag
 *  - Updating astTagGetStr
 *  - Adding a constructor, presumably
 *  - Adding some code to the parser, presumably
 *  - Handling it in:
 *    - analyzer*.c
 *      - isNodeLvalue if a value tag
 *    - eval.c
 *    - emitter*.c
 */
typedef enum astTag {
    astUndefined,
    astInvalid,
    astMarker,
    astEmpty,
    astModule,
    astUsing,
    astFnImpl,
    astType, astDecl, astParam, astStruct, astUnion, astEnum, astConst,
    astCode, astBranch, astLoop, astIter, astReturn, astBreak, astContinue,
    astBOP, astUOP, astTOP, astIndex, astCall, astCast, astSizeof, astLiteral,
    astVAStart, astVAEnd, astVAArg, astVACopy, astAssert, astEllipsis
} astTag;

typedef enum markerTag {
    markerUndefined,
    markerAuto,
    markerStatic,
    markerExtern,
    markerArrayDesignatedInit,
    markerStructDesignatedInit
} markerTag;

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
    opIndex, opCall,
    opPreIncrement, opPreDecrement, opPostIncrement, opPostDecrement,
    opMember, opMemberDeref,
    opAssert
} opTag;

typedef enum literalTag {
    literalUndefined,
    literalIdent,
    literalInt,
    literalChar,
    literalBool,
    literalStr,
    literalCompound,
    literalInit,
    literalLambda
} literalTag;

/**
 * Syntax tree node
 *
 * Subclasses:
 *   o Values
 *      - Defined as those that are returned by parserValue.
 *      - They represent value returning expressions.
 *      - Handled by functions such as analyzerValue, emitterValue.
 *
 *   o DeclExprs
 *      - Defined as those that are returned by parserDeclExpr.
 *      - One of: BOP, UOP, Index, Call, Const, Literal
 *      - Represent direct declarators, in the language of the standard.
 *      - Handlers such as analyzerDeclNode.
 *
 * These subclasses are mutually exclusive.
 *
 * Invariants:
 *   - location is the location of the first token in the grammatical
 *     construction, or another location appropriate for errors to
 *     appear at. For example, the location of a BOP will be the operator.
 *
 *   - For Values:
 *      - Any child is itself a value, other than those of
 *         - Sizeof, Cast, Literal[lit=Init, Compound, Lambda], VAArg
 *      - After analysis, dt will be a type representing the result of
 *        the expression.
 *
 *   - For DeclExprs:
 *      - Any child is itself a DeclExpr, other than
 *         - The right child of a BOP[o=Assign],
 *         - The right child of an Index,
 *         - The linked list children of a Call.
 *      - After parsing, if the DeclExpr declares a symbol it will be
 *        stored in symbol.
 *
 * Owns:
 *   - All children
 *      - Except for the right child of an astUsing.
 *   - dt
 *   - literal
 */
typedef struct ast {
    astTag tag;

    tokenLocation location;

    /*Linked list for container nodes like a module or parameter list and children of containers*/
    ast* firstChild;
    ast* lastChild;
    ast* nextSibling;
    ast* prevSibling;
    int children;

    /*Binary tree*/
    ast* l;
    opTag o;
    ast* r;      /*Always used for unary operators*/
    type* dt;    /*Result data type*/

    sym* symbol;

    union {
        /*(DeclExpr) astLiteral[lit=Ident]*/
        storageTag storage;
        /*(DeclExpr) astBOP[o=Assign]
          astMarker[m=ArrayDesignator]*/
        intptr_t constant;
    };

    union {
        /*astMarker*/
        markerTag marker;
        /*astLiteral*/
        struct {
            literalTag litTag;
            void* literal;
        };
    };
} ast;

ast* astCreate (astTag tag, tokenLocation location);
void astDestroy (ast* Node);

ast* astCreateInvalid (tokenLocation location);
ast* astCreateMarker (tokenLocation location, markerTag marker);
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

ast* astCreateBOP (tokenLocation location, ast* l, opTag o, ast* r);
ast* astCreateUOP (tokenLocation location, opTag o, ast* r);
ast* astCreateTOP (tokenLocation location, ast* cond, ast* l, ast* r);
ast* astCreateIndex (tokenLocation location, ast* base, ast* index);
ast* astCreateCall (tokenLocation location, ast* function);
ast* astCreateCast (tokenLocation location, ast* result, ast* r);
ast* astCreateSizeof (tokenLocation location, ast* r);
ast* astCreateLiteral (tokenLocation location, literalTag litTag);
ast* astCreateLiteralIdent (tokenLocation location, char* ident);
ast* astCreateAssert (tokenLocation location, ast* expr);

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
