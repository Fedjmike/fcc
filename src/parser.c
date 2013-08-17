#include "../inc/parser.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/sym.h"
#include "../inc/ast.h"

#include "../inc/lexer.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"
#include "../inc/parser-decl.h"

#include "stdlib.h"
#include "string.h"

static ast* parserModule (parserCtx* ctx);

static ast* parserLine (parserCtx* ctx);

static ast* parserIf (parserCtx* ctx);
static ast* parserWhile (parserCtx* ctx);
static ast* parserDoWhile (parserCtx* ctx);
static ast* parserFor (parserCtx* ctx);

static parserCtx* parserInit (const char* File, sym* Global) {
    parserCtx* ctx = malloc(sizeof(parserCtx));

    ctx->lexer = lexerInit(File);

    ctx->location.line = ctx->lexer->stream->line;
    ctx->location.lineChar = ctx->lexer->stream->lineChar;

    ctx->scope = Global;

    ctx->breakLevel = 0;

    ctx->errors = 0;
    ctx->warnings = 0;

    return ctx;
}

static void parserEnd (parserCtx* ctx) {
    lexerEnd(ctx->lexer);
    free(ctx);
}

parserResult parser (const char* File, sym* Global) {
    parserCtx* ctx = parserInit(File, Global);

    ast* Module = parserModule(ctx);

    parserResult result = {Module, ctx->errors, ctx->warnings};

    parserEnd(ctx);

    return result;
}

/**
 * Module = [{ Decl# | ";" }]
 */
static ast* parserModule (parserCtx* ctx) {
    debugEnter("Module");

    ast* Module = astCreate(astModule, ctx->location);
    Module->symbol = ctx->scope;

    while (ctx->lexer->token != tokenEOF) {
        if (tokenTryMatchPunct(ctx, punctSemicolon))
            astAddChild(Module, astCreateEmpty(ctx->location));

        else
            astAddChild(Module, parserDecl(ctx, true));

        //debugWait();
    }

    debugLeave();

    return Module;
}

/**
 * Code = ( "{" [{ Line }] "}" ) | Line
 */
ast* parserCode (parserCtx* ctx) {
    debugEnter("Code");

    ast* Node = astCreate(astCode, ctx->location);
    Node->symbol = symCreateScope(ctx->scope);
    sym* OldScope = scopeSet(ctx, Node->symbol);

    if (tokenTryMatchPunct(ctx, punctLBrace))
        while (!tokenTryMatchPunct(ctx, punctRBrace) && ctx->lexer->token != tokenEOF)
            astAddChild(Node, parserLine(ctx));

    else
        astAddChild(Node, parserLine(ctx));

    ctx->scope = OldScope;

    debugLeave();

    return Node;
}

/**
 * Line = If | While | DoWhile | For | Code | Decl# | ( [ ( "return" Value ) | "break" | Value ] ";" )
 */
static ast* parserLine (parserCtx* ctx) {
    debugEnter("Line");

    ast* Node = 0;

    if (tokenIsKeyword(ctx, keywordIf))
        Node = parserIf(ctx);

    else if (tokenIsKeyword(ctx, keywordWhile))
        Node = parserWhile(ctx);

    else if (tokenIsKeyword(ctx, keywordDo))
        Node = parserDoWhile(ctx);

    else if (tokenIsKeyword(ctx, keywordFor))
        Node = parserFor(ctx);

    else if (tokenIsPunct(ctx, punctLBrace))
        Node = parserCode(ctx);

    else if (tokenIsDecl(ctx))
        Node = parserDecl(ctx, false);

    /*Statements (that which require ';')*/
    else {
        if (tokenTryMatchKeyword(ctx, keywordReturn)) {
            Node = astCreate(astReturn, ctx->location);

            if (!tokenIsPunct(ctx, punctSemicolon))
                Node->r = parserValue(ctx);

        } else if (tokenIsKeyword(ctx, keywordBreak)) {
            if (ctx->breakLevel == 0)
                errorIllegalOutside(ctx, "break", "a loop");

            else
                tokenMatch(ctx);

            Node = astCreate(astBreak, ctx->location);

        /*Allow empty lines, ";"*/
        } else if (tokenIsPunct(ctx, punctSemicolon))
            Node = astCreateEmpty(ctx->location);

        else
            Node = parserValue(ctx);

        tokenMatchPunct(ctx, punctSemicolon);
    }

    debugLeave();

    //debugWait();

    return Node;
}

/**
 * If = "if" "(" Value ")" Code [ "else" Code ]
 */
static ast* parserIf (parserCtx* ctx) {
    debugEnter("If");

    ast* Node = astCreate(astBranch, ctx->location);

    tokenMatchKeyword(ctx, keywordIf);
    tokenMatchPunct(ctx, punctLParen);
    astAddChild(Node, parserValue(ctx));
    tokenMatchPunct(ctx, punctRParen);

    Node->l = parserCode(ctx);

    if (tokenTryMatchKeyword(ctx, keywordElse))
        Node->r = parserCode(ctx);

    debugLeave();

    return Node;
}

/**
 * While = "while" "(" Value ")" Code
 */
static ast* parserWhile (parserCtx* ctx) {
    debugEnter("While");

    ast* Node = astCreate(astLoop, ctx->location);

    tokenMatchKeyword(ctx, keywordWhile);
    tokenMatchPunct(ctx, punctLParen);
    Node->l = parserValue(ctx);
    tokenMatchPunct(ctx, punctRParen);

    ctx->breakLevel++;
    Node->r = parserCode(ctx);
    ctx->breakLevel--;

    debugLeave();

    return Node;
}

/**
 * DoWhile = "do" Code "while" "(" Value ")" ";"
 */
static ast* parserDoWhile (parserCtx* ctx) {
    debugEnter("DoWhile");

    ast* Node = astCreate(astLoop, ctx->location);

    tokenMatchKeyword(ctx, keywordDo);

    ctx->breakLevel++;
    Node->l = parserCode(ctx);
    ctx->breakLevel--;

    tokenMatchKeyword(ctx, keywordWhile);
    tokenMatchPunct(ctx, punctLParen);
    Node->r = parserValue(ctx);
    tokenMatchPunct(ctx, punctRParen);
    tokenMatchPunct(ctx, punctSemicolon);

    debugLeave();

    return Node;
}

/**
 * For := "for" "(" Decl# | ( [ Value ] ";" ) [ Value ] ";" [ Value ] ")" Code
 */
static ast* parserFor (parserCtx* ctx) {
    debugEnter("For");

    ast* Node = astCreate(astIter, ctx->location);

    tokenMatchKeyword(ctx, keywordFor);
    tokenMatchPunct(ctx, punctLParen);

    Node->symbol = symCreateScope(ctx->scope);
    sym* OldScope = scopeSet(ctx, Node->symbol);

    /*Initializer*/
    if (tokenIsDecl(ctx))
        astAddChild(Node, parserDecl(ctx, false));

    else {
        if (!tokenIsPunct(ctx, punctSemicolon))
            astAddChild(Node, parserValue(ctx));

        else
            astAddChild(Node, astCreateEmpty(ctx->location));

        tokenMatchPunct(ctx, punctSemicolon);
    }

    /*Condition*/
    if (!tokenIsPunct(ctx, punctSemicolon))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreateEmpty(ctx->location));

    tokenMatchPunct(ctx, punctSemicolon);

    /*Iterator*/
    if (!tokenIsPunct(ctx, punctRParen))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreateEmpty(ctx->location));

    tokenMatchPunct(ctx, punctRParen);

    ctx->breakLevel++;
    Node->l = parserCode(ctx);
    ctx->breakLevel--;

    ctx->scope = OldScope;

    debugLeave();

    return Node;
}
