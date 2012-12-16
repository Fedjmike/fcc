#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserAssign (parserCtx* ctx);
static ast* parserTernary (parserCtx* ctx);
static ast* parserBool (parserCtx* ctx);
static ast* parserEquality (parserCtx* ctx);
static ast* parserRel (parserCtx* ctx);
static ast* parserExpr (parserCtx* ctx);
static ast* parserTerm (parserCtx* ctx);
static ast* parserUnary (parserCtx* ctx);
static ast* parserObject (parserCtx* ctx);
static ast* parserFactor (parserCtx* ctx);

/**
 * Value = Assign
 */
ast* parserValue (parserCtx* ctx) {
    debugEnter("Value");

    ast* Node = parserAssign(ctx);

    debugLeave();

    return Node;
}

/**
 * Assign = Ternary [ "=" | "+=" | "-=" | "*=" | "/=" Assign ]
 */
static ast* parserAssign (parserCtx* ctx) {
    debugEnter("Assign");

    ast* Node = parserTernary(ctx);

    if  (tokenIs(ctx, "=") ||
         tokenIs(ctx, "+=") || tokenIs(ctx, "-=") ||
         tokenIs(ctx, "*=") || tokenIs(ctx, "/=")) {
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
 * Bool = Equality [{ "&&" | "||" Equality }]
 */
static ast* parserBool (parserCtx* ctx) {
    debugEnter("Bool");

    ast* Node = parserEquality(ctx);

    while (tokenIs(ctx,  "&&") || tokenIs(ctx, "||")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserEquality(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Equality = Rel [{ "==" | "!=" Rel }]
 */
static ast* parserEquality (parserCtx* ctx) {
    debugEnter("Equality");

    ast* Node = parserRel(ctx);

    while (tokenIs(ctx, "==") || tokenIs(ctx, "!=")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserRel(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Rel = Expr [{ ">" | ">=" | "<" | "<=" Expr }]
 */
static ast* parserRel (parserCtx* ctx) {
    debugEnter("Rel");

    ast* Node = parserExpr(ctx);

    while (tokenIs(ctx, ">") || tokenIs(ctx, ">=") ||
           tokenIs(ctx, "<") || tokenIs(ctx, "<=")) {
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
 * Term = Unary [{ "*" | "/" Unary }]
 */
static ast* parserTerm (parserCtx* ctx) {
    debugEnter("Term");

    ast* Node = parserUnary(ctx);

    while (tokenIs(ctx, "*") || tokenIs(ctx, "/")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(ctx->location, Node, o, parserUnary(ctx));
    }

    debugLeave();

    return Node;
}

/**
 * Unary = ( "!" | "-" | "*" | "&" Unary ) | Object [{ "++" | "--" }]
 */
static ast* parserUnary (parserCtx* ctx) {
    /* Interestingly, this function makes extensive use of itself */

    debugEnter("Unary");

    ast* Node = 0;

    if (tokenIs(ctx, "!") ||
        tokenIs(ctx, "-") ||
        tokenIs(ctx, "*") ||
        tokenIs(ctx, "&")) {
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
                      | ( "." <Ident> )
                      | ( "->" <Ident> ) }]
 */
static ast* parserObject (parserCtx* ctx) {
    debugEnter("Object");

    ast* Node = parserFactor(ctx);

    while (tokenIs(ctx, "[") || tokenIs(ctx, ".") || tokenIs(ctx, "->")) {
        /*Array or pointer indexing*/
        if (tokenTryMatchStr(ctx, "[")) {
            Node = astCreateIndex(ctx->location, Node, parserValue(ctx));
            tokenMatchStr(ctx, "]");

        /*struct[*] member access*/
        } else /*if (tokenIs(ctx, ".") || tokenIs(ctx, "->"))*/ {
            ast* tmp = Node;
            Node = astCreate(astBOP, ctx->location);
            Node->o = tokenDupMatch(ctx);
            Node->l = tmp;

            /*Is the right hand a valid symbol?*/

            Node->r = astCreate(astLiteral, ctx->location);
            Node->r->litClass = literalIdent;
            Node->r->literal = (void*) strdup(ctx->lexer->buffer);
            Node->symbol = Node->r->symbol = symChild(Node->l->symbol->dt.basic,
                                                      (char*) Node->r->literal);

            if (Node->r->symbol)
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
            | <Int>
            | ( <Ident> [ "(" [ Value [{ "," Value }] ] ")" ] )
 */
static ast* parserFactor (parserCtx* ctx) {
    debugEnter("Factor");

    ast* Node = 0;

    /*Parenthesized expression*/
    if (tokenTryMatchStr(ctx, "(")) {
        Node = parserValue(ctx);
        tokenMatchStr(ctx, ")");

    /*Integer literal*/
    } else if (ctx->lexer->token == tokenInt) {
        Node = astCreateLiteral(ctx->location, literalInt);
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = tokenMatchInt(ctx);

    /*Boolean literal*/
    } else if (tokenIs(ctx, "true") || tokenIs(ctx, "false")) {
        Node = astCreateLiteral(ctx->location, literalBool);
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = tokenIs(ctx, "true") ? 1 : 0;

        tokenMatch(ctx);

    /*Identifier or function call*/
    } else if (ctx->lexer->token == tokenIdent) {
        Node = astCreateLiteral(ctx->location, literalIdent);
        Node->literal = (void*) strdup(ctx->lexer->buffer);
        Node->symbol = symFind(ctx->scope, (char*) Node->literal);

        /*Valid symbol?*/
        if (Node->symbol) {
            tokenMatch(ctx);
            Node->dt = Node->symbol->dt;

            reportSymbol(Node->symbol);

        } else {
            errorUndefSym(ctx);
            tokenNext(ctx);

            Node->dt.basic = symFindGlobal("int");
        }

        /*Actually it was a function call*/
        if (tokenIs(ctx, "(")) {
            tokenMatch(ctx);

            Node = astCreateCall(ctx->location, Node);

            /*Eat params*/
            if (!tokenIs(ctx, ")")) do {
                astAddChild(Node, parserValue(ctx));
            } while (tokenTryMatchStr(ctx, ","));

            tokenMatchStr(ctx, ")");
        }

    } else {
        errorExpected(ctx, "expression");
        tokenNext(ctx);
    }

    debugLeave();

    return Node;
}
