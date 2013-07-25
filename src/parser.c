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
 * Module = [{ ModuleDecl | ";" }]
 */
static ast* parserModule (parserCtx* ctx) {
    debugEnter("Module");

    ast* Module = astCreate(astModule, ctx->location);
    Module->symbol = ctx->scope;

    while (ctx->lexer->token != tokenEOF) {
        if (tokenTryMatchStr(ctx, ";"))
            astAddChild(Module, astCreateEmpty(ctx->location));

        else
            astAddChild(Module, parserModuleDecl(ctx));

        debugWait();
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

    if (tokenTryMatchStr(ctx, "{")) {
        while (!tokenIs(ctx, "}"))
            astAddChild(Node, parserLine(ctx));

        tokenMatch(ctx);

    } else
        astAddChild(Node, parserLine(ctx));

    ctx->scope = OldScope;

    debugLeave();

    return Node;
}

/**
 * Line = If | While | DoWhile | For | Code | [ ( "return" Value ) | "break" | Decl | Value ] ";"
 */
static ast* parserLine (parserCtx* ctx) {
    debugEnter("Line");

    ast* Node = 0;

    if (tokenIs(ctx, "if"))
        Node = parserIf(ctx);

    else if (tokenIs(ctx, "while"))
        Node = parserWhile(ctx);

    else if (tokenIs(ctx, "do"))
        Node = parserDoWhile(ctx);

    else if (tokenIs(ctx, "for"))
        Node = parserFor(ctx);

    else if (tokenIs(ctx, "{"))
        Node = parserCode(ctx);

    /*Statements (that which require ';')*/
    else {
        if (tokenTryMatchStr(ctx, "return")) {
            Node = astCreate(astReturn, ctx->location);

            if (!tokenIs(ctx, ";"))
                Node->r = parserValue(ctx);

        } else if (tokenIs(ctx, "break")) {
            if (ctx->breakLevel == 0)
                errorIllegalOutside(ctx, "break", "a loop");

            else
                tokenMatch(ctx);

            Node = astCreate(astBreak, ctx->location);

        /*Allow empty lines, ";"*/
        } else if (tokenIs(ctx, ";"))
            Node = astCreateEmpty(ctx->location);

        else if (tokenIsDecl(ctx))
            Node = parserDecl(ctx);

        else
            Node = parserValue(ctx);

        tokenMatchStr(ctx, ";");
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

    tokenMatchStr(ctx, "if");
    tokenMatchStr(ctx, "(");
    astAddChild(Node, parserValue(ctx));
    tokenMatchStr(ctx, ")");

    Node->l = parserCode(ctx);

    if (tokenTryMatchStr(ctx, "else"))
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

    tokenMatchStr(ctx, "while");
    tokenMatchStr(ctx, "(");
    Node->l = parserValue(ctx);
    tokenMatchStr(ctx, ")");

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

    tokenMatchStr(ctx, "do");

    ctx->breakLevel++;
    Node->l = parserCode(ctx);
    ctx->breakLevel--;

    tokenMatchStr(ctx, "while");
    tokenMatchStr(ctx, "(");
    Node->r = parserValue(ctx);
    tokenMatchStr(ctx, ")");
    tokenMatchStr(ctx, ";");

    debugLeave();

    return Node;
}

/**
 * For := "for" "(" [ Decl | Value ] ";" [ Value ] ";" [ Value ] ")" Code
 */
static ast* parserFor (parserCtx* ctx) {
    debugEnter("For");

    ast* Node = astCreate(astIter, ctx->location);

    tokenMatchStr(ctx, "for");
    tokenMatchStr(ctx, "(");

    Node->symbol = symCreateScope(ctx->scope);
    sym* OldScope = scopeSet(ctx, Node->symbol);

    /*Initializer*/
    if (!tokenIs(ctx, ";")) {
        if (tokenIsDecl(ctx))
            astAddChild(Node, parserDecl(ctx));

        else
            astAddChild(Node, parserValue(ctx));

    } else
        astAddChild(Node, astCreateEmpty(ctx->location));

    tokenMatchStr(ctx, ";");

    /*Condition*/
    if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreateEmpty(ctx->location));

    tokenMatchStr(ctx, ";");

    /*Iterator*/
    if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreateEmpty(ctx->location));

    tokenMatchStr(ctx, ")");

    ctx->breakLevel++;
    Node->l = parserCode(ctx);
    ctx->breakLevel--;

    ctx->scope = OldScope;

    debugLeave();

    return Node;
}
