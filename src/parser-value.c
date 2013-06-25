#include "../inc/parser-value.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"
#include "../inc/ast.h"

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

    while (tokenIs(ctx, ",")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserAssign(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Assign = Ternary [ "=" | "+=" | "-=" | "*=" | "/=" | "^=" Assign ]
 */
static ast* parserAssign (parserCtx* ctx) {
    debugEnter("Assign");

    ast* Node = parserTernary(ctx);

    if  (   tokenIs(ctx, "=")
         || tokenIs(ctx, "+=") || tokenIs(ctx, "-=")
         || tokenIs(ctx, "*=") || tokenIs(ctx, "/=")
         || tokenIs(ctx, "^=")) {
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

    if (tokenTryMatchStr(ctx, "?")) {
        ast* l = parserTernary(ctx);
        tokenMatchStr(ctx, ":");
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

    while (   tokenIs(ctx,  "&&") || tokenIs(ctx, "||")
           || tokenIs(ctx,  "&") || tokenIs(ctx, "|")
           || tokenIs(ctx,  "^")) {
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

    while (   tokenIs(ctx, "==") || tokenIs(ctx, "!=")
           || tokenIs(ctx, ">") || tokenIs(ctx, ">=")
           || tokenIs(ctx, "<") || tokenIs(ctx, "<=")) {
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

    while (tokenIs(ctx, ">>") || tokenIs(ctx, "<<")) {
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

    while (tokenIs(ctx, "+") || tokenIs(ctx, "-")) {
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

    while (   tokenIs(ctx, "*") || tokenIs(ctx, "/")
           || tokenIs(ctx, "/")) {
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
    /* Interestingly, this function makes extensive use of itself */

    debugEnter("Unary");

    ast* Node = 0;

    if (   tokenIs(ctx, "!")
        || tokenIs(ctx, "~")
        || tokenIs(ctx, "-")
        || tokenIs(ctx, "*")
        || tokenIs(ctx, "&")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(ctx->location, o, parserUnary(ctx));

    } else
        Node = parserObject(ctx);

    while (tokenIs(ctx, "++") || tokenIs(ctx, "--"))
        Node = astCreateUOP(ctx->location, tokenDupMatch(ctx), Node);

    debugLeave();

    return Node;
}

/**
 * Object = Factor [{   ( "[" Value "]" )
 *                    | ( "(" [ AssignValue [{ "," AssignValue }] ] ")" )
 *                    | ( "." <Ident> )
 *                    | ( "->" <Ident> ) }]
 */
static ast* parserObject (parserCtx* ctx) {
    debugEnter("Object");

    ast* Node = parserFactor(ctx);

    while (   tokenIs(ctx, "[") || tokenIs(ctx, "(")
           || tokenIs(ctx, ".") || tokenIs(ctx, "->")) {
        /*Array or pointer indexing*/
        if (tokenTryMatchStr(ctx, "[")) {
            Node = astCreateIndex(ctx->location, Node, parserValue(ctx));
            tokenMatchStr(ctx, "]");

        /*Function call*/
        } else if (tokenTryMatchStr(ctx, "(")) {
            Node = astCreateCall(ctx->location, Node);

            /*Eat params*/
            if (!tokenIs(ctx, ")")) do {
                astAddChild(Node, parserAssignValue(ctx));
            } while (tokenTryMatchStr(ctx, ","));

            tokenMatchStr(ctx, ")");

        /*struct[*] member access*/
        } else /*if (tokenIs(ctx, ".") || tokenIs(ctx, "->"))*/ {
            tokenLocation loc = ctx->location;
            char* o = tokenDupMatch(ctx);
            Node = astCreateBOP(loc, Node, o,
                                astCreateLiteral(ctx->location, literalIdent));
            Node->r->literal = (void*) strdup(ctx->lexer->buffer);

            if (tokenIsIdent(ctx))
                tokenMatch(ctx);

            else {
                errorExpected(ctx, "field name");
                tokenNext(ctx);
            }
        }
    }

    debugLeave();

    return Node;
}

/**
 * Factor =   ( "(" Value ")" )
 *          | ( "(" Type ")" Unary )
 *          | ( "{" [ AssignValue [{ "," AssignValue }] ] "}" )
 *          | ( "sizeof" ( "(" Type | Value ")" ) | Value
 *          | <Int>
 *          | <Bool>
 *          | <Str>
 *          | <Ident>
 */
static ast* parserFactor (parserCtx* ctx) {
    debugEnter("Factor");

    ast* Node = 0;

    /*Cast or parenthesized expression*/
    if (tokenTryMatchStr(ctx, "(")) {
        /*Cast*/
        if (tokenIsDecl(ctx)) {
            debugMode old = debugSetMode(debugFull);
            Node = astCreateCast(ctx->location, parserType(ctx));
            debugSetMode(old);
            tokenMatchStr(ctx, ")");
            Node->r = parserUnary(ctx);

        /*Expression*/
        } else {
            Node = parserValue(ctx);
            tokenMatchStr(ctx, ")");
        }

    /*Struct/array literal*/
    } else if (tokenTryMatchStr(ctx, "{")) {
        Node = astCreateLiteral(ctx->location, literalArray);

        do {
            astAddChild(Node, parserAssignValue(ctx));
        } while (tokenTryMatchStr(ctx, ","));

        tokenMatchStr(ctx, "}");

    /*Sizeof*/
    } else if (tokenTryMatchStr(ctx, "sizeof")) {
        /*Even if an lparen is encountered, it could still be a
          parenthesized expression*/
        if (tokenTryMatchStr(ctx, "(")) {
            if (tokenIsDecl(ctx))
                Node = parserType(ctx);

            else
                Node = parserValue(ctx);

            tokenMatchStr(ctx, ")");

        } else
            Node = parserValue(ctx);

        Node = astCreateSizeof(ctx->location, Node);

    /*Integer literal*/
    } else if (tokenIsInt(ctx)) {
        Node = astCreateLiteral(ctx->location, literalInt);
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = tokenMatchInt(ctx);

    /*Boolean literal*/
    } else if (tokenIs(ctx, "true") || tokenIs(ctx, "false")) {
        Node = astCreateLiteral(ctx->location, literalBool);
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = tokenIs(ctx, "true") ? 1 : 0;

        tokenMatch(ctx);

    /*String literal*/
    } else if (tokenIsString(ctx)) {
        Node = astCreateLiteral(ctx->location, literalStr);
        Node->literal = calloc(ctx->lexer->length, sizeof(char));

        /*Iterate through the string excluding the first and last
          characters - the quotes*/
        for (int i = 1, length = 0; i+2 < ctx->lexer->length; i++) {
            /*Escape sequence*/
            if (ctx->lexer->buffer[i] == '\\') {
                i++;

                if (   ctx->lexer->buffer[i] == 'n' || ctx->lexer->buffer[i] == 'r'
                    || ctx->lexer->buffer[i] == 't' || ctx->lexer->buffer[i] == '\\'
                    || ctx->lexer->buffer[i] == '\'' || ctx->lexer->buffer[i] == '"') {
                    ((char*) Node->literal)[length++] = '\\';
                    ((char*) Node->literal)[length++] = ctx->lexer->buffer[i];

                /*An actual linebreak mid string? Escaped, ignore it*/
                } else if (   ctx->lexer->buffer[i] == '\n'
                         || ctx->lexer->buffer[i] == '\r')
                    i++;

                /*Unrecognised escape: ignore*/
                else
                    i++;

            } else
                ((char*) Node->literal)[length++] = ctx->lexer->buffer[i];
        }

        tokenMatch(ctx);

    /*Identifier*/
    } else if (tokenIsIdent(ctx)) {
        sym* Symbol = symFind(ctx->scope, (char*) ctx->lexer->buffer);

        /*Valid symbol?*/
        if (Symbol) {
            Node = astCreateLiteral(ctx->location, literalIdent);
            Node->literal = (void*) tokenDupMatch(ctx);
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
