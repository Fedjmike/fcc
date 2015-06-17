#include "../inc/parser-value.h"

#include "../inc/debug.h"
#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/error.h"

#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-decl.h"

#include "stdlib.h"
#include "string.h"
#include "assert.h"

using "../inc/debug.h";
using "../inc/sym.h";
using "../inc/ast.h";
using "../inc/error.h";

using "../inc/lexer.h";
using "../inc/parser-helpers.h";
using "../inc/parser-decl.h";

using "stdlib.h";
using "string.h";

static ast* parserComma (parserCtx* ctx);
static ast* parserAssign (parserCtx* ctx);
static ast* parserTernary (parserCtx* ctx);
static ast* parserBool (parserCtx* ctx);
static ast* parserEquality (parserCtx* ctx);
static ast* parserShift (parserCtx* ctx);
static ast* parserExpr (parserCtx* ctx);
static ast* parserTerm (parserCtx* ctx);
static ast* parserUnary (parserCtx* ctx);
static ast* parserPostUnary (parserCtx* ctx);
static ast* parserObject (parserCtx* ctx);
static ast* parserFactor (parserCtx* ctx);
static ast* parserElementInit (parserCtx* ctx);
static ast* parserDesignatedInit (parserCtx* ctx, ast* element, bool array);
static ast* parserLambda (parserCtx* ctx, bool partial);
static ast* parserVA (parserCtx* ctx);

/**
 * Value = Comma
 */
ast* parserValue (parserCtx* ctx) {
    debugEnter("Value");
    debugMode old = debugSetMode(debugMinimal);

    ast* Node = parserComma(ctx);

    debugSetMode(old);
    debugLeave();

    return Node;
}

/**
 * AssignValue = Assign
 */
ast* parserAssignValue (parserCtx* ctx) {
    debugEnter("AssignValue");
    debugMode old = debugSetMode(debugMinimal);

    ast* Node = parserAssign(ctx);

    debugSetMode(old);
    debugLeave();

    return Node;
}

/**
 * Comma = Assign [{ "," Assign }]
 */
static ast* parserComma (parserCtx* ctx) {
    debugEnter("Comma");

    ast* Node = parserAssign(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctComma) ? opComma : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserAssign(ctx));

    debugLeave();

    return Node;
}

/**
 * Assign = Ternary [ "&=" | "|=" | "^=" | ">>=" | "<<=" | "=" | "+=" | "-=" | "*=" | "/=" | "%=" Assign ]
 */
static ast* parserAssign (parserCtx* ctx) {
    debugEnter("Assign");

    ast* Node = parserTernary(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    if ((o = tokenTryMatchPunct(ctx, punctBitwiseAndAssign) ? opBitwiseAndAssign :
             tokenTryMatchPunct(ctx, punctBitwiseOrAssign) ? opBitwiseOrAssign :
             tokenTryMatchPunct(ctx, punctBitwiseXorAssign) ? opBitwiseXorAssign :
             tokenTryMatchPunct(ctx, punctShrAssign) ? opShrAssign :
             tokenTryMatchPunct(ctx, punctShlAssign) ? opShlAssign :
             tokenTryMatchPunct(ctx, punctAssign) ? opAssign :
             tokenTryMatchPunct(ctx, punctPlusAssign) ? opAddAssign :
             tokenTryMatchPunct(ctx, punctMinusAssign) ? opSubtractAssign :
             tokenTryMatchPunct(ctx, punctTimesAssign) ? opMultiplyAssign :
             tokenTryMatchPunct(ctx, punctDivideAssign) ? opDivideAssign :
             tokenTryMatchPunct(ctx, punctModuloAssign) ? opModuloAssign : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserAssign(ctx));

    debugLeave();

    return Node;
}

/**
 * Ternary = Bool [ "?" Value ":" Ternary ]
 */
static ast* parserTernary (parserCtx* ctx) {
    debugEnter("Ternary");

    ast* Node = parserBool(ctx);
    tokenLocation loc = ctx->location;

    if (tokenTryMatchPunct(ctx, punctQuestion)) {
        ast* l = parserValue(ctx);
        tokenMatchPunct(ctx, punctColon);
        ast* r = parserTernary(ctx);

        Node = astCreateTOP(loc, Node, l, r);
    }

    debugLeave();

    return Node;
}

/**
 * Bool = Equality [{ "&&" | "||" | "&" | "|" | "^" Equality }]
 */
static ast* parserBool (parserCtx* ctx) {
    debugEnter("Bool");

    ast* Node = parserEquality(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctLogicalAnd) ? opLogicalAnd :
                tokenTryMatchPunct(ctx, punctLogicalOr) ? opLogicalOr :
                tokenTryMatchPunct(ctx, punctBitwiseAnd) ? opBitwiseAnd :
                tokenTryMatchPunct(ctx, punctBitwiseOr) ? opBitwiseOr :
                tokenTryMatchPunct(ctx, punctBitwiseXor) ? opBitwiseXor : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserEquality(ctx));

    debugLeave();

    return Node;
}

/**
 * Equality = Shift [{ "==" | "!=" | ">" | ">=" | "<" | "<=" Shift }]
 */
static ast* parserEquality (parserCtx* ctx) {
    debugEnter("Equality");

    ast* Node = parserShift(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctEqual) ? opEqual :
                tokenTryMatchPunct(ctx, punctNotEqual) ? opNotEqual :
                tokenTryMatchPunct(ctx, punctGreater) ? opGreater :
                tokenTryMatchPunct(ctx, punctGreaterEqual) ? opGreaterEqual :
                tokenTryMatchPunct(ctx, punctLess) ? opLess :
                tokenTryMatchPunct(ctx, punctLessEqual) ? opLessEqual : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserShift(ctx));

    debugLeave();

    return Node;
}

/**
 * Shift = Expr [{ ">>" | "<<" Expr }]
 */
static ast* parserShift (parserCtx* ctx) {
    debugEnter("Shift");

    ast* Node = parserExpr(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctShr) ? opShr :
                tokenTryMatchPunct(ctx, punctShl) ? opShl : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserExpr(ctx));

    debugLeave();

    return Node;
}

/**
 * Expr = Term [{ "+" | "-" Term }]
 */
static ast* parserExpr (parserCtx* ctx) {
    debugEnter("Expr");

    ast* Node = parserTerm(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctPlus) ? opAdd :
                tokenTryMatchPunct(ctx, punctMinus) ? opSubtract : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserTerm(ctx));

    debugLeave();

    return Node;
}

/**
 * Term = Unary [{ "*" | "/" | "%" Unary }]
 */
static ast* parserTerm (parserCtx* ctx) {
    debugEnter("Term");

    ast* Node = parserUnary(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctTimes) ? opMultiply :
                tokenTryMatchPunct(ctx, punctDivide) ? opDivide :
                tokenTryMatchPunct(ctx, punctModulo) ? opModulo : opUndefined))
        Node = astCreateBOP(loc, Node, o, parserUnary(ctx));

    debugLeave();

    return Node;
}

/**
 * Unary = ( "!" | "~" | "+" | "-" | "*" | "&" | "++" | "--" Unary ) | PostUnary
 */
static ast* parserUnary (parserCtx* ctx) {
    debugEnter("Unary");

    ast* Node;
    tokenLocation loc = ctx->location;
    opTag o;

    if ((o = tokenTryMatchPunct(ctx, punctLogicalNot) ? opLogicalNot :
             tokenTryMatchPunct(ctx, punctBitwiseNot) ? opBitwiseNot :
             tokenTryMatchPunct(ctx, punctPlus) ? opUnaryPlus :
             tokenTryMatchPunct(ctx, punctMinus) ? opNegate :
             tokenTryMatchPunct(ctx, punctTimes) ? opDeref :
             tokenTryMatchPunct(ctx, punctBitwiseAnd) ? opAddressOf :
             tokenTryMatchPunct(ctx, punctPlusPlus) ? opPreIncrement :
             tokenTryMatchPunct(ctx, punctMinusMinus) ? opPreDecrement : opUndefined))
        Node = astCreateUOP(loc, o, parserUnary(ctx));

    else
        Node = parserPostUnary(ctx);

    debugLeave();

    return Node;
}

/**
 * PostUnary = Object [{ "++" | "--" }]
 */
static ast* parserPostUnary (parserCtx* ctx) {
    debugEnter("PostUnary");

    /*Interestingly, this call to parserObject parses itself*/
    ast* Node = parserObject(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while ((o = tokenTryMatchPunct(ctx, punctPlusPlus) ? opPostIncrement :
                tokenTryMatchPunct(ctx, punctMinusMinus) ? opPostDecrement : opUndefined))
        Node = astCreateUOP(loc, o, Node);

    debugLeave();

    return Node;
}

/**
 * Object = Factor [{   ( "[" Value "]" )
 *                    | ( "(" [ AssignValue [{ "," AssignValue }] ] ")" )
 *                    | ( "." | "->" <Ident> ) }]
 */
static ast* parserObject (parserCtx* ctx) {
    debugEnter("Object");

    ast* Node = parserFactor(ctx);
    tokenLocation loc = ctx->location;
    opTag o;

    while (true) {
        /*Array or pointer indexing*/
        if (tokenTryMatchPunct(ctx, punctLBracket)) {
            Node = astCreateIndex(loc, Node, parserValue(ctx));
            tokenMatchPunct(ctx, punctRBracket);

        /*Function call*/
        } else if (tokenTryMatchPunct(ctx, punctLParen)) {
            Node = astCreateCall(loc, Node);

            /*Eat params*/
            if (!tokenIsPunct(ctx, punctRParen)) do {
                astAddChild(Node, parserAssignValue(ctx));
            } while (tokenTryMatchPunct(ctx, punctComma));

            tokenMatchPunct(ctx, punctRParen);

        /*struct[*] member access*/
        } else if ((o = tokenTryMatchPunct(ctx, punctPeriod) ? opMember :
                        tokenTryMatchPunct(ctx, punctArrow) ? opMemberDeref : opUndefined)) {
            ast* field = astCreateLiteral(loc, literalIdent);
            Node = astCreateBOP(loc, Node, o, field);
            field->literal = (void*) strdup(ctx->lexer->buffer);

            if (tokenIsIdent(ctx))
                tokenMatch(ctx);

            else
                errorExpected(ctx, "field name");

        } else
            break;
    }

    debugLeave();

    return Node;
}

/**
 * Factor =   ( "(" Value ")" )
 *          | ( "(" Type ")" Unary )
 *          | ( [ "(" Type ")" ] "{" [ ElementInit [{ "," ElementInit }] ] "}" )
 *          | ( "sizeof" ( "(" Type | Value ")" ) | Unary )
 *          | VAStart | VAEnd | VAArg | VACopy | ( "assert" "(" AssignValue ")" )
 *          | Lambda | <Int> | <Bool> | <Str> | <Char> | <Ident>
 */
static ast* parserFactor (parserCtx* ctx) {
    debugEnter("Factor");

    ast* Node;

    tokenLocation loc = ctx->location;

    /*Cast, compound literal or parenthesized expression*/
    if (tokenTryMatchPunct(ctx, punctLParen)) {
        /*Cast or compound literal*/
        if (tokenIsDecl(ctx)) {
            Node = parserType(ctx);
            tokenMatchPunct(ctx, punctRParen);

            /*Compound literal*/
            if (tokenTryMatchPunct(ctx, punctLBrace)) {
                ast* tmp = Node;
                Node = astCreateLiteral(loc, literalCompound);
                Node->symbol = symCreateNamed(symId, ctx->scope, "");
                Node->l = tmp;

                if (!tokenIsPunct(ctx, punctRBrace)) do {
                    astAddChild(Node, parserElementInit(ctx));
                } while (tokenTryMatchPunct(ctx, punctComma));

                tokenMatchPunct(ctx, punctRBrace);

            /*Cast*/
            } else
                Node = astCreateCast(loc, Node, parserUnary(ctx));

        /*Expression*/
        } else {
            Node = parserValue(ctx);
            tokenMatchPunct(ctx, punctRParen);
        }

    /*Struct/array initializer*/
    } else if (tokenTryMatchPunct(ctx, punctLBrace)) {
        Node = astCreateLiteral(loc, literalInit);

        if (!tokenIsPunct(ctx, punctRBrace)) do {
            astAddChild(Node, parserElementInit(ctx));
        } while (tokenTryMatchPunct(ctx, punctComma));

        tokenMatchPunct(ctx, punctRBrace);

    /*Sizeof*/
    } else if (tokenTryMatchKeyword(ctx, keywordSizeof)) {
        /*Even if an lparen is encountered, it could still be a
          parenthesized expression*/
        if (tokenTryMatchPunct(ctx, punctLParen)) {
            if (tokenIsDecl(ctx))
                Node = parserType(ctx);

            else
                Node = parserValue(ctx);

            tokenMatchPunct(ctx, punctRParen);

        } else
            Node = parserUnary(ctx);

        Node = astCreateSizeof(loc, Node);

    /*Lambda*/
    } else if (tokenIsPunct(ctx, punctLBracket)) {
        Node = parserLambda(ctx, false);

    /*Integer*/
    } else if (tokenIsInt(ctx)) {
        Node = astCreateLiteral(ctx->location, literalInt);
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = tokenMatchInt(ctx);

    /*Boolean*/
    } else if (tokenIsKeyword(ctx, keywordTrue) || tokenIsKeyword(ctx, keywordFalse)) {
        Node = astCreateLiteral(ctx->location, literalBool);
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = (char)(tokenIsKeyword(ctx, keywordTrue) ? 1 : 0);

        tokenMatch(ctx);

    /*String*/
    } else if (tokenIsString(ctx)) {
        Node = astCreateLiteral(ctx->location, literalStr);
        Node->literal = (void*) tokenMatchStr(ctx);

    /*Character*/
    } else if (tokenIsChar(ctx)) {
        Node = astCreateLiteral(ctx->location, literalChar);
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = tokenMatchChar(ctx);

    /*va_start va_end va_arg va_copy*/
    } else if (   tokenIsKeyword(ctx, keywordVAStart)
               || tokenIsKeyword(ctx, keywordVAEnd)
               || tokenIsKeyword(ctx, keywordVAArg)
               || tokenIsKeyword(ctx, keywordVACopy)) {
        Node = parserVA(ctx);

    /*assert*/
    } else if (tokenTryMatchKeyword(ctx, keywordAssert)) {
        tokenMatchPunct(ctx, punctLParen);
        Node = astCreateAssert(loc, parserAssignValue(ctx));
        tokenMatchPunct(ctx, punctRParen);

    /*Identifier*/
    } else if (tokenIsIdent(ctx)) {
        sym* Symbol = symFind(ctx->scope, (char*) ctx->lexer->buffer);

        /*Valid symbol?*/
        if (Symbol) {
            Node = astCreateLiteral(ctx->location, literalIdent);
            Node->literal = (char*) tokenDupMatch(ctx);
            Node->symbol = Symbol;

        } else {
            Node = astCreateInvalid(ctx->location);
            errorUndefSym(ctx);
            tokenNext(ctx);
        }

    } else {
        Node = astCreateInvalid(ctx->location);
        errorExpected(ctx, "expression");
        tokenNext(ctx);
    }

    debugLeave();

    return Node;
}

/**
 * ElementInit = [   (   ( "." <Ident> )
 *                     | ( "[" Value "]" ) DesignatedInit )
 *                 | AssignValue ]
 */
static ast* parserElementInit (parserCtx* ctx) {
    debugEnter("ElementInit");

    tokenLocation loc = ctx->location;
    ast* Node;

    /*Skipped field/element*/
    if (tokenIsPunct(ctx, punctComma) || tokenIsPunct(ctx, punctRBrace))
        Node = astCreateEmpty(loc);

    /*Struct designated initializer*/
    else if (tokenTryMatchPunct(ctx, punctPeriod)) {
        ast* field = astCreateLiteral(loc, literalIdent);
        field->literal = (void*) strdup(ctx->lexer->buffer);

        if (tokenIsIdent(ctx))
            tokenMatch(ctx);

        else
            errorExpected(ctx, "field name");

        Node = parserDesignatedInit(ctx, field, false);

    /*Array designated initializer, or the beginning of a lambda*/
    } else if (tokenTryMatchPunct(ctx, punctLBracket)) {
        /*Lambda*/
        if (tokenIsPunct(ctx, punctRBracket))
            Node = parserLambda(ctx, true);

        /*Designated initializer*/
        else {
            ast* element = parserValue(ctx);
            tokenMatchPunct(ctx, punctRBracket);

            Node = parserDesignatedInit(ctx, element, true);
        }

    /*Regular value*/
    } else
        Node = parserAssignValue(ctx);

    debugLeave();

    return Node;
}

/**
 * DesignatedInit = "=" AssignValue
 */
static ast* parserDesignatedInit (parserCtx* ctx, ast* element, bool array) {
    debugEnter("DesignatedInit");

    tokenLocation loc = ctx->location;
    tokenMatchPunct(ctx, punctAssign);

    ast* Node = astCreateMarker(loc, array ? markerArrayDesignatedInit
                                           : markerStructDesignatedInit);
    Node->l = element;
    Node->r = parserAssignValue(ctx);

    debugLeave();

    return Node;
}

/**
 * Lambda = "[" "]" ParamList
 *          ( "{" Code "}" ) | ( "(" Value ")" )
 */
static ast* parserLambda (parserCtx* ctx, bool partial) {
    debugEnter("Lambda");

    ast* Node = astCreateLiteral(ctx->location, literalLambda);
    Node->symbol = symCreateNamed(symId, ctx->module, "");
    sym* oldScope = scopeSet(ctx, Node->symbol);

    /*Capture (not supported yet)*/

    if (!partial)
        tokenMatchPunct(ctx, punctLBracket);

    tokenMatchPunct(ctx, punctRBracket);

    /*Params*/

    parserParamList(ctx, Node, true);

    /*Body*/

    if (tokenIsPunct(ctx, punctLBrace))
        Node->r = parserCode(ctx);

    else if (tokenTryMatchPunct(ctx, punctLParen)) {
        Node->r = parserValue(ctx);
        tokenTryMatchPunct(ctx, punctRParen);

    } else {
        errorExpected(ctx, "lambda body");
        Node->r = astCreateInvalid(ctx->location);
    }

    ctx->scope = oldScope;

    debugLeave();

    return Node;
}

/**
 * VAStart = va_start "(" AssignValue "," <Ident> ")"
 * VAEnd = va_end "(" AssignValue ")"
 * VAArg = va_arg "(" AssignValue "," Type ")"
 * VACopy = va_copy "(" AssignValue "," AssignValue ")"
 */
static ast* parserVA (parserCtx* ctx) {
    debugEnter("VA");

    /*Parse all builtins in one, remembering which keyword was found.*/

    tokenLocation loc = ctx->location;

    astTag tag =   tokenTryMatchKeyword(ctx, keywordVAStart) ? astVAStart
                 : tokenTryMatchKeyword(ctx, keywordVAEnd) ? astVAEnd
                 : tokenTryMatchKeyword(ctx, keywordVACopy) ? astVACopy
                 : (tokenMatchKeyword(ctx, keywordVAArg), astVAArg);

    ast* Node = astCreate(tag, loc);

    tokenMatchPunct(ctx, punctLParen);

    Node->l = parserAssignValue(ctx);

    if (tag == astVAEnd)
        ;

    else {
        tokenMatchPunct(ctx, punctComma);

        /*va_start takes a parameter name
          Validate it as an ident but don't validate the symbol*/
        if (tag == astVAStart) {
            sym* Symbol = symFind(ctx->scope, (char*) ctx->lexer->buffer);

            if (tokenIsIdent(ctx) && Symbol) {
                Node->r = astCreateLiteral(ctx->location, literalIdent);
                Node->r->literal = (char*) tokenDupMatch(ctx);
                Node->r->symbol = Symbol;

            } else {
                errorExpected(ctx, "parameter name");
                Node->r = astCreateInvalid(ctx->location);
            }

        } else if (tag == astVAArg)
            Node->r = parserType(ctx);

        else {
            assert(tag == astVACopy);
            Node->r = parserAssignValue(ctx);
        }
    }

    tokenMatchPunct(ctx, punctRParen);

    debugLeave();

    return Node;
}
