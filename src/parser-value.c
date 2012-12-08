#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserAssign (parserCtx* ctx);
static ast* parserTernary (parserCtx* ctx );
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
    puts("Value+");

    ast* Node = parserAssign(ctx);

    puts("-");

    return Node;
}

/**
 * Assign = Ternary [ "=" | "+=" | "-=" | "*=" | "/=" Assign ]
 */
ast* parserAssign (parserCtx* ctx) {
    puts("Assign+");

    ast* Node = parserTernary(ctx);

    if  (tokenIs(ctx, "=") ||
         tokenIs(ctx, "+=") || tokenIs(ctx, "-=") || tokenIs(ctx, "*=") || tokenIs(ctx, "/=")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(Node, o, parserAssign(ctx));
    }

    puts("-");

    return Node;
}

/**
 * Ternary = Bool [ "?" Ternary ":" Ternary ]
 */
ast* parserTernary (parserCtx* ctx ) {
    puts("Ternary+");

    ast* Node = parserBool(ctx);

    if (tokenTryMatchStr(ctx, "?")) {
        ast* l = parserTernary(ctx);
        tokenMatchStr(ctx, ":");
        ast* r = parserTernary(ctx);

        Node = astCreateTOP(Node, l, r);
    }

    puts("-");

    return Node;
}

/**
 * Bool = Equality [{ "&&" | "||" Equality }]
 */
ast* parserBool (parserCtx* ctx) {
    puts("Bool+");

    ast* Node = parserEquality(ctx);

    while (tokenIs(ctx,  "&&") || tokenIs(ctx, "||")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(Node, o, parserEquality(ctx));
    }

    puts("-");

    return Node;
}

/**
 * Equality = Rel [{ "==" | "!=" Rel }]
 */
ast* parserEquality (parserCtx* ctx) {
    puts("Equality+");

    ast* Node = parserRel(ctx);

    while (tokenIs(ctx, "==") || tokenIs(ctx, "!=")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(Node, o, parserRel(ctx));
    }

    puts("-");

    return Node;
}

/**
 * Rel = Expr [{ ">" | ">=" | "<" | "<=" Expr }]
 */
ast* parserRel (parserCtx* ctx) {
    puts("Rel+");

    ast* Node = parserExpr(ctx);

    while (tokenIs(ctx, ">") || tokenIs(ctx, ">=") || tokenIs(ctx, "<") || tokenIs(ctx, "<=")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(Node, o, parserExpr(ctx));
    }

    puts("-");

    return Node;
}

/**
 * Expr = Term [{ "+" | "-" Term }]
 */
ast* parserExpr (parserCtx* ctx) {
    puts("Expr+");

    ast* Node = parserTerm(ctx);

    while (tokenIs(ctx, "+") || tokenIs(ctx, "-")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(Node, o, parserTerm(ctx));
    }

    puts("-");

    return Node;
}

/**
 * Term = Unary [{ "*" | "/" Unary }]
 */
ast* parserTerm (parserCtx* ctx) {
    puts("Term+");

    ast* Node = parserUnary(ctx);

    while (tokenIs(ctx, "*") || tokenIs(ctx, "/")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(Node, o, parserUnary(ctx));
    }

    puts("-");

    return Node;
}

/**
 * Unary = ( "!" | "-" | "*" | "&" | "++" | "--" Unary ) | Object [{ "++" | "--" }]
 */
ast* parserUnary (parserCtx* ctx) {
    /* Interestingly, this function makes extensive use of itself */

    puts("Unary+");

    ast* Node = 0;

    if (tokenIs(ctx, "!") ||
        tokenIs(ctx, "-") ||
        tokenIs(ctx, "*") ||
        tokenIs(ctx, "&") ||
        tokenIs(ctx, "++") ||
        tokenIs(ctx, "--")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(o, parserUnary(ctx));

        if (!strcmp(Node->o, "*")) {
            if (typeIsPtr(Node->r->dt)) {
                Node->dt = typeDerefPtr(Node->r->dt);

            } else
                errorInvalidOpExpected(ctx, "dereference", "pointer", Node->r->dt);
        }

    } else
        Node = parserObject(ctx);

    while (tokenIs(ctx, "++") || tokenIs(ctx, "--")) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(o, Node);
    }

    puts("-");

    return Node;
}

/**
 * Object = Factor [{   ( "[" Value "]" )
                      | ( "." <Ident> )
                      | ( "->" <Ident> ) }]
 */
ast* parserObject (parserCtx* ctx) {
    puts("Object+");

    ast* Node = parserFactor(ctx);

    while (tokenIs(ctx, "[") || tokenIs(ctx, ".") || tokenIs(ctx, "->")) {
        /*Array or pointer indexing*/
        if (tokenIs(ctx, "[")) {
            tokenMatch(ctx);

            ast* tmp = Node;
            Node = astCreate(astIndex);
            Node->l = tmp;
            Node->r = parserValue(ctx);

            if (typeIsArray(Node->l->dt))
                Node->dt = typeIndexArray(Node->l->dt);

            else if (typeIsPtr(Node->l->dt))
                Node->dt = typeDerefPtr(Node->l->dt);

            else
                errorInvalidOpExpected(ctx, "indexing", "array or pointer", Node->l->dt);

            tokenMatchStr(ctx, "]");

        /*struct[*] member access*/
        } else /*if (tokenIs(ctx, ".") || tokenIs(ctx, "->"))*/ {
            ast* tmp = Node;
            Node = astCreate(astBOP);
            Node->o = strdup(ctx->lexer->buffer);
            Node->l = tmp;

            /*Was the left hand a valid operand?*/
            if (typeIsRecord(Node->l->dt)) {
                errorInvalidOpExpected(ctx, "member access", "record type", Node->l->dt);
                tokenNext(ctx);

            } else if (!strcmp("->", Node->o) && Node->l->dt.ptr != 1) {
                errorInvalidOpExpected(ctx, "member dereference", "struct pointer", Node->l->dt);
                tokenNext(ctx);

            } else if (!strcmp(".", Node->o) && Node->l->dt.ptr != 0) {
                errorInvalidOp(ctx, "member access", "struct pointer", Node->l->dt);
                tokenNext(ctx);

            } else
                tokenMatch(ctx);

            /*Is the right hand a valid operand?*/
            if (ctx->lexer->token == tokenIdent) {
                Node->r = astCreate(astLiteral);
                Node->r->litClass = literalIdent;
                Node->r->literal = (void*) strdup(ctx->lexer->buffer);
                Node->r->symbol = symChild(Node->l->dt.basic, (char*) Node->r->literal);

                if (Node->r->symbol == 0) {
                    errorExpected(ctx, "field name");
                    tokenNext(ctx);
                    Node->r->dt.basic = symFindGlobal("int");

                } else {
                    tokenMatch(ctx);
                    Node->r->dt = Node->r->symbol->dt;
                    reportType(Node->r->dt);
                }

            } else {
                errorExpected(ctx, "field name");
                tokenNext(ctx);
                Node->r->dt.basic = symFindGlobal("int");
            }

            /*Propogate type*/
            Node->dt = Node->r->dt;
        }
    }

    puts("-");

    return Node;
}

/**
 * Factor =   ( "(" Value ")" )
            | <Int>
            | ( <Ident> [ "(" [ Value [{ "," Value }] ] ")" ] )
 */
ast* parserFactor (parserCtx* ctx) {
    puts("Factor+");

    ast* Node = 0;

    /*Parenthesized expression*/
    if (tokenIs(ctx, "(")) {
        tokenMatch(ctx);
        Node = parserValue(ctx);
        tokenMatchStr(ctx, ")");

    /*Integer literal*/
    } else if (ctx->lexer->token == tokenInt) {
        Node = astCreate(astLiteral);
        Node->dt.basic = symFindGlobal("int");
        Node->litClass = literalInt;
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = tokenMatchInt(ctx);

    /*Boolean literal*/
    } else if (tokenIs(ctx, "true") || tokenIs(ctx, "false")) {
        Node = astCreate(astLiteral);
        Node->dt.basic = symFindGlobal("bool");
        Node->litClass = literalBool;
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = tokenIs(ctx, "true") ? 1 : 0;

        tokenMatch(ctx);

    /*Identifier or function call*/
    } else if (ctx->lexer->token == tokenIdent) {
        Node = astCreate(astLiteral);
        Node->litClass = literalIdent;
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

            ast* tmp = Node;
            Node = astCreate(astCall);
            Node->l = tmp;

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

    puts("-");

    return Node;
}
