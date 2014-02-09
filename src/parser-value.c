#include "../inc/parser-value.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/error.h"

#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-decl.h"

#include "stdlib.h"
#include "string.h"

static ast* parserComma (parserCtx* ctx);
static ast* parserAssign (parserCtx* ctx);
static ast* parserTernary (parserCtx* ctx);
static ast* parserBool (parserCtx* ctx);
static ast* parserEquality (parserCtx* ctx);
static ast* parserShift (parserCtx* ctx);
static ast* parserExpr (parserCtx* ctx);
static ast* parserTerm (parserCtx* ctx);
static ast* parserUnary (parserCtx* ctx);
static ast* parserObject (parserCtx* ctx);
static ast* parserFactor (parserCtx* ctx);

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

    while (tokenIsPunct(ctx, punctComma)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserAssign(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Assign = Ternary [ "=" | "+=" | "-=" | "*=" | "/=" | "|=" | "^=" Assign ]
 */
static ast* parserAssign (parserCtx* ctx) {
    debugEnter("Assign");

    ast* Node = parserTernary(ctx);

    if  (   tokenIsPunct(ctx, punctAssign)
         || tokenIsPunct(ctx, punctPlusAssign) || tokenIsPunct(ctx, punctMinusAssign)
         || tokenIsPunct(ctx, punctTimesAssign) || tokenIsPunct(ctx, punctDivideAssign)
         || tokenIsPunct(ctx, punctBitwiseOrAssign) || tokenIsPunct(ctx, punctBitwiseXorAssign)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserAssign(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Ternary = Bool [ "?" Ternary ":" Ternary ]
 */
static ast* parserTernary (parserCtx* ctx ) {
    debugEnter("Ternary");

    ast* Node = parserBool(ctx);

    if (tokenTryMatchPunct(ctx, punctQuestion)) {
        ast* l = parserTernary(ctx);
        tokenMatchPunct(ctx, punctColon);
        ast* r = parserTernary(ctx);

        Node = astCreateTOP(ctx->location, Node, l, r);
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

    while (   tokenIsPunct(ctx, punctLogicalAnd) || tokenIsPunct(ctx, punctLogicalOr)
           || tokenIsPunct(ctx, punctBitwiseAnd) || tokenIsPunct(ctx, punctBitwiseOr)
           || tokenIsPunct(ctx, punctBitwiseXor)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserEquality(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Equality = Shift [{ "==" | "!=" | ">" | ">=" | "<" | "<=" Shift }]
 */
static ast* parserEquality (parserCtx* ctx) {
    debugEnter("Equality");

    ast* Node = parserShift(ctx);

    while (   tokenIsPunct(ctx, punctEqual) || tokenIsPunct(ctx, punctNotEqual)
           || tokenIsPunct(ctx, punctGreater) || tokenIsPunct(ctx, punctGreaterEqual)
           || tokenIsPunct(ctx, punctLess) || tokenIsPunct(ctx, punctLessEqual)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserShift(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Shift = Expr [{ ">>" | "<<" Expr }]
 */
static ast* parserShift (parserCtx* ctx) {
    debugEnter("Shift");

    ast* Node = parserExpr(ctx);

    while (tokenIsPunct(ctx, punctShr) || tokenIsPunct(ctx, punctShl)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserExpr(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Expr = Term [{ "+" | "-" Term }]
 */
static ast* parserExpr (parserCtx* ctx) {
    debugEnter("Expr");

    ast* Node = parserTerm(ctx);

    while (tokenIsPunct(ctx, punctPlus) || tokenIsPunct(ctx, punctMinus)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserTerm(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Term = Unary [{ "*" | "/" | "%" Unary }]
 */
static ast* parserTerm (parserCtx* ctx) {
    debugEnter("Term");

    ast* Node = parserUnary(ctx);

    while (   tokenIsPunct(ctx, punctTimes) || tokenIsPunct(ctx, punctDivide)
           || tokenIsPunct(ctx, punctModulo)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserUnary(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Unary = ( "!" | "~" | "-" | "*" | "&" Unary ) | Object [{ "++" | "--" }]
 */
static ast* parserUnary (parserCtx* ctx) {
    debugEnter("Unary");

    ast* Node = 0;

    if (   tokenIsPunct(ctx, punctLogicalNot) || tokenIsPunct(ctx, punctBitwiseNot)
        || tokenIsPunct(ctx, punctMinus)
        || tokenIsPunct(ctx, punctTimes) || tokenIsPunct(ctx, punctBitwiseAnd)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(ctx->location, o, parserUnary(ctx));

    } else
        /*Interestingly, this call to parserObject parses itself*/
        Node = parserObject(ctx);

    while (tokenIsPunct(ctx, punctPlusPlus) || tokenIsPunct(ctx, punctMinusMinus))
        Node = astCreateUOP(ctx->location, tokenDupMatch(ctx), Node);

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

    while (true) {
        /*Array or pointer indexing*/
        if (tokenTryMatchPunct(ctx, punctLBracket)) {
            Node = astCreateIndex(ctx->location, Node, parserValue(ctx));
            tokenMatchPunct(ctx, punctRBracket);

        /*Function call*/
        } else if (tokenTryMatchPunct(ctx, punctLParen)) {
            Node = astCreateCall(ctx->location, Node);

            /*Eat params*/
            if (!tokenIsPunct(ctx, punctRParen)) do {
                astAddChild(Node, parserAssignValue(ctx));
            } while (tokenTryMatchPunct(ctx, punctComma));

            tokenMatchPunct(ctx, punctRParen);

        /*struct[*] member access*/
        } else if (tokenIsPunct(ctx, punctPeriod) || tokenIsPunct(ctx, punctArrow)) {
            tokenLocation loc = ctx->location;
            char* o = tokenDupMatch(ctx);
            Node = astCreateBOP(loc, Node, o,
                                astCreateLiteral(ctx->location, literalIdent));
            Node->r->literal = (void*) strdup(ctx->lexer->buffer);

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
 *          | ( [ "(" Type ")" ] "{" [ AssignValue [{ "," AssignValue }] ] "}" )
 *          | ( "sizeof" ( "(" Type | Value ")" ) | Value )
 *          | <Int> | <Bool> | <Str> | <Char> | <Ident>
 */
static ast* parserFactor (parserCtx* ctx) {
    debugEnter("Factor");

    ast* Node = 0;

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

                do {
                    astAddChild(Node, parserAssignValue(ctx));
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

    /*Struct/array initialiazer*/
    } else if (tokenTryMatchPunct(ctx, punctLBrace)) {
        Node = astCreateLiteral(loc, literalInit);

        do {
            astAddChild(Node, parserAssignValue(ctx));
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

    /*Integer*/
    } else if (tokenIsInt(ctx)) {
        Node = astCreateLiteral(ctx->location, literalInt);
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = tokenMatchInt(ctx);

    /*Boolean*/
    } else if (tokenIsKeyword(ctx, keywordTrue) || tokenIsKeyword(ctx, keywordFalse)) {
        Node = astCreateLiteral(ctx->location, literalBool);
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = tokenIsKeyword(ctx, keywordTrue) ? 1 : 0;

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
