#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-value.h"

static ast* parserAssign (sym* Scope);
static ast* parserTernary (sym* Scope );
static ast* parserBool (sym* Scope);
static ast* parserEquality (sym* Scope);
static ast* parserRel (sym* Scope);
static ast* parserExpr (sym* Scope);
static ast* parserTerm (sym* Scope);
static ast* parserUnary (sym* Scope);
static ast* parserObject (sym* Scope);
static ast* parserFactor (sym* Scope);

/**
 * Value = Assign
 */
ast* parserValue (sym* Scope) {
    puts("Value+");

    ast* Node = parserAssign(Scope);

    puts("-");

    return Node;
}

/**
 * Assign = Ternary [ "=" | "+=" | "-=" | "*=" | "/=" Assign ]
 */
ast* parserAssign (sym* Scope) {
    puts("Assign+");

    ast* Node = parserTernary(Scope);

    if  (lexerIs("=") ||
         lexerIs("+=") || lexerIs("-=") || lexerIs("*=") || lexerIs("/=")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserAssign(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Ternary = Bool [ "?" Ternary ":" Ternary ]
 */
ast* parserTernary (sym* Scope ) {
    puts("Ternary+");

    ast* Node = parserBool(Scope);

    if (lexerTryMatchStr("?")) {
        ast* l = parserTernary(Scope);
        lexerMatch(":");
        ast* r = parserTernary(Scope);

        Node = astCreateTOP(Node, l, r);
    }

    puts("-");

    return Node;
}

/**
 * Bool = Equality [{ "&&" | "||" Equality }]
 */
ast* parserBool (sym* Scope) {
    puts("Bool+");

    ast* Node = parserEquality(Scope);

    while (lexerIs( "&&") || lexerIs("||")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserEquality(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Equality = Rel [{ "==" | "!=" Rel }]
 */
ast* parserEquality (sym* Scope) {
    puts("Equality+");

    ast* Node = parserRel(Scope);

    while (lexerIs("==") || lexerIs("!=")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserRel(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Rel = Expr [{ ">" | ">=" | "<" | "<=" Expr }]
 */
ast* parserRel (sym* Scope) {
    puts("Rel+");

    ast* Node = parserExpr(Scope);

    while (lexerIs(">") || lexerIs(">=") || lexerIs("<") || lexerIs("<=")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserExpr(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Expr = Term [{ "+" | "-" Term }]
 */
ast* parserExpr (sym* Scope) {
    puts("Expr+");

    ast* Node = parserTerm(Scope);

    while (lexerIs("+") || lexerIs("-")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserTerm(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Term = Unary [{ "*" | "/" Unary }]
 */
ast* parserTerm (sym* Scope) {
    puts("Term+");

    ast* Node = parserUnary(Scope);

    while (lexerIs("*") || lexerIs("/")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserUnary(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Unary = ( "!" | "-" | "*" | "&" | "++" | "--" Unary ) | Object [{ "++" | "--" }]
 */
ast* parserUnary (sym* Scope) {
    /* Interestingly, this function makes extensive use of itself */

    puts("Unary+");

    ast* Node = 0;

    if (lexerIs("!") ||
        lexerIs("-") ||
        lexerIs("*") ||
        lexerIs("&") ||
        lexerIs("++") ||
        lexerIs("--")) {
        char* o = lexerDupMatch();
        Node = astCreateUOP(o, parserUnary(Scope));

        if (!strcmp(Node->o, "*")) {
            if (typeIsPtr(Node->r->dt)) {
                Node->dt = typeDerefPtr(Node->r->dt);

            } else
                errorInvalidOpExpected("dereference", "pointer", Node->r->dt);
        }

    } else
        Node = parserObject(Scope);

    while (lexerIs("++") || lexerIs("--")) {
        char* o = lexerDupMatch();
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
ast* parserObject (sym* Scope) {
    puts("Object+");

    ast* Node = parserFactor(Scope);

    while (lexerIs("[") || lexerIs(".") || lexerIs("->")) {
        /*Array or pointer indexing*/
        if (lexerIs("[")) {
            lexerMatch();

            ast* tmp = Node;
            Node = astCreate(astIndex);
            Node->l = tmp;
            Node->r = parserValue(Scope);

            if (typeIsArray(Node->l->dt))
                Node->dt = typeIndexArray(Node->l->dt);

            else if (typeIsPtr(Node->l->dt))
                Node->dt = typeDerefPtr(Node->l->dt);

            else
                errorInvalidOpExpected("indexing", "array or pointer", Node->l->dt);

            lexerMatchStr("]");

        /*struct[*] member access*/
        } else /*if (lexerIs(".") || lexerIs("->"))*/ {
            ast* tmp = Node;
            Node = astCreate(astBOP);
            Node->o = strdup(lexerBuffer);
            Node->l = tmp;

            /*Was the left hand a valid operand?*/
            if (typeIsRecord(Node->l->dt)) {
                errorInvalidOpExpected("member access", "record type", Node->l->dt);
                lexerNext();

            } else if (!strcmp("->", Node->o) && Node->l->dt.ptr != 1) {
                errorInvalidOpExpected("member dereference", "struct pointer", Node->l->dt);
                lexerNext();

            } else if (!strcmp(".", Node->o) && Node->l->dt.ptr != 0) {
                errorInvalidOp("member access", "struct pointer", Node->l->dt);
                lexerNext();

            } else
                lexerMatch();

            /*Is the right hand a valid operand?*/
            if (lexerToken == tokenIdent) {
                Node->r = astCreate(astLiteral);
                Node->r->litClass = literalIdent;
                Node->r->literal = (void*) strdup(lexerBuffer);
                Node->r->symbol = symChild(Node->l->dt.basic, (char*) Node->r->literal);

                if (Node->r->symbol == 0) {
                    errorExpected("field name");
                    lexerNext();
                    Node->r->dt.basic = symFindGlobal("int");

                } else {
                    lexerMatch();
                    Node->r->dt = Node->r->symbol->dt;
                    reportType(Node->r->dt);
                }

            } else {
                errorExpected("field name");
                lexerNext();
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
ast* parserFactor (sym* Scope) {
    puts("Factor+");

    ast* Node = 0;

    /*Parenthesized expression*/
    if (lexerIs("(")) {
        lexerMatch();
        Node = parserValue(Scope);
        lexerMatchStr(")");

    /*Integer literal*/
    } else if (lexerToken == tokenInt) {
        Node = astCreate(astLiteral);
        Node->dt.basic = symFindGlobal("int");
        Node->litClass = literalInt;
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = lexerMatchInt();

    /*Boolean literal*/
    } else if (lexerIs("true") || lexerIs("false")) {
        Node = astCreate(astLiteral);
        Node->dt.basic = symFindGlobal("bool");
        Node->litClass = literalBool;
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = lexerIs("true") ? 1 : 0;

        lexerMatch();

    /*Identifier or function call*/
    } else if (lexerToken == tokenIdent) {
        Node = astCreate(astLiteral);
        Node->litClass = literalIdent;
        Node->literal = (void*) strdup(lexerBuffer);
        Node->symbol = symFind(Scope, (char*) Node->literal);

        /*Valid symbol?*/
        if (Node->symbol) {
            lexerMatch();
            Node->dt = Node->symbol->dt;

            reportSymbol(Node->symbol);

        } else {
            errorUndefSym();
            lexerNext();

            Node->dt.basic = symFindGlobal("int");
        }

        /*Actually it was a function call*/
        if (lexerIs("(")) {
            lexerMatch();

            ast* tmp = Node;
            Node = astCreate(astCall);
            Node->l = tmp;

            /*Eat params*/
            if (!lexerIs(")")) do {
                astAddChild(Node, parserValue(Scope));
            } while (lexerTryMatchStr(","));

            lexerMatchStr(")");
        }

    } else {
        errorExpected("expression");
        lexerNext();
    }

    puts("-");

    return Node;
}
