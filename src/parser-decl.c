#include "../inc/parser-decl.h"

#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/debug.h"

#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserDeclField (parserCtx* ctx);
static ast* parserDeclParam (parserCtx* ctx, bool inDecl);

static ast* parserDeclBasic (parserCtx* ctx);

static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, bool inParam);
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, bool inParam);
static ast* parserDeclFunction (parserCtx* ctx, bool inDecl, bool inParam, ast* atom);
static ast* parserDeclAtom (parserCtx* ctx, bool inDecl, bool inParam);

static ast* parserName (parserCtx* ctx);

/**
 * DeclStruct = "struct" <Ident> [ "{" [{ DeclField ";" }] "}" ] ";"
 */
struct ast* parserDeclStruct (parserCtx* ctx) {
    debugEnter("Struct");

    tokenMatchStr(ctx, "struct");

    ast* Node = astCreateDeclStruct(ctx->location, parserName(ctx));

    sym* Scope = ctx->scope;
    sym* Symbol = ctx->scope = symCreateStruct(Scope, (char*) Node->l->literal);

    /*Body?*/
    if (tokenTryMatchStr(ctx, "{")) {
        /*Eat fields*/
        while (!tokenIs(ctx, "}")) {
            astAddChild(Node, parserDeclField(ctx));
            tokenMatchStr(ctx, ";");
        }

        tokenMatchStr(ctx, "}");

    /*No? Just a prototype*/
    } else
        Symbol->proto = true;

    tokenMatchStr(ctx, ";");

    /*Revert to the old scope*/
    ctx->scope = Scope;

    debugLeave();

    return Node;
}

/**
 * Type = DeclBasic DeclExpr*
 *
 * DeclExpr is told not to allow identifiers or initiations, and not
 * to create symbols.
 */
 ast* parserType (parserCtx* ctx) {
    debugEnter("Type");

    tokenLocation loc = ctx->location;
    ast* basic = parserDeclBasic(ctx);
    ast* expr = parserDeclExpr(ctx, false, false);
    ast* Node = astCreateType(loc, basic, expr);

    debugLeave();

    return Node;
}

/**
 * Decl = DeclBasic DeclExpr# [{ "," DeclExpr# }]#
 *
 * DeclExpr is told to require identifiers and to create symbols.
 * Initiations are allowed, and a function implementation is allowed
 * (as the first and only DeclExpr, in that case).
 */
ast* parserDecl (parserCtx* ctx) {
    debugEnter("Decl");

    tokenLocation loc = ctx->location;
    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));

    do {
        astAddChild(Node, parserDeclExpr(ctx, true, false));
    } while (tokenTryMatchStr(ctx, ","));

    debugLeave();

    return Node;
}

/**
 * ModuleDecl = DeclBasic DeclExpr# [{ "," DeclExpr# }]# ";"#
 */
ast* parserModuleDecl (parserCtx* ctx) {
    debugEnter("ModuleDecl");

    tokenLocation loc = ctx->location;
    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));

    astAddChild(Node, parserDeclExpr(ctx, true, false));

    if (tokenIs(ctx, "{")) {
        /*!!!ALLOWED?!!!*/
        sym* Scope = ctx->scope;
        loc = ctx->location;

        ctx->scope = Node->lastChild->symbol;
        Node = astCreateFnImpl(loc, Node, parserCode(ctx));
        Node->symbol = Node->l->firstChild->symbol;
        ctx->scope = Scope;

    } else {
        while (tokenTryMatchStr(ctx, ","))
            astAddChild(Node, parserDeclExpr(ctx, true, false));

        tokenMatchStr(ctx, ";");
    }

    debugLeave();

    return Node;
}

/**
 * DeclField = Decl
 */
static ast* parserDeclField (parserCtx* ctx) {
    return parserDecl(ctx);
    /*!!!CHANGEZ MOI!!!
      To what?
      Differences: not allowed to assign values, not allowed to decl
                   functions, certainly not implement them (fine)*/
}

/**
 * DeclParam = DeclBasic DeclExpr#
 *
 * DeclExpr is told to accept but not require identifiers and symbol
 * creation if and only if inDecl is true.
 */
static ast* parserDeclParam (parserCtx* ctx, bool inDecl) {
    debugEnter("DeclParam");

    tokenLocation loc = ctx->location;
    ast* basic = parserDeclBasic(ctx);
    ast* expr = parserDeclExpr(ctx, inDecl, true);
    ast* Node = astCreateDeclParam(loc, basic, expr);

    debugLeave();

    return Node;
}

/**
 * DeclBasic = <Ident>
 */
static ast* parserDeclBasic (parserCtx* ctx) {
    debugEnter("DeclBasic");

    ast* Node = 0;

    sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

    if (Symbol) {
        Node = astCreateLiteralIdent(ctx->location, tokenDupMatch(ctx));
        Node->symbol = Symbol;

    } else {
        if (tokenIsIdent(ctx))
            errorUndefSym(ctx);

        else
            errorExpected(ctx, "type name");

        tokenNext(ctx);
    }

    debugLeave();

    return Node;
}

/**
 * DeclExpr = ( "*" DeclExpr ) | DeclObject [ "=" Value ]
 */
static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, bool inParam) {
    debugEnter("DeclExpr");

    ast* Node = 0;

    if (tokenIs(ctx, "*")) {
        tokenLocation loc = ctx->location;
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(loc, o, parserDeclExpr(ctx, inDecl, inParam));
        Node->symbol = Node->r->symbol;

    } else
        Node = parserDeclObject(ctx, inDecl, inParam);

    if (inDecl && tokenIs(ctx, "=")) {
        /*!!!ALLOWED?!!!*/
        sym* Symbol = Node->symbol;
        tokenLocation loc = ctx->location;
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(loc, Node, o, parserValue(ctx));
        Node->symbol = Symbol;
    }

    debugLeave();

    return Node;
}

/**
 * DeclObject = DeclAtom [{ DeclFunction | ( "[" Value "]" ) }]
 */
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, bool inParam) {
    debugEnter("DeclObject");

    ast* Node = parserDeclAtom(ctx, inDecl, inParam);

    while (tokenIs(ctx, "(") || tokenIs(ctx, "[")) {
        tokenLocation loc = ctx->location;

        /*Function*/
        if (tokenIs(ctx, "("))
            Node = parserDeclFunction(ctx, inDecl, inParam, Node);

        /*Array*/
        else if (tokenTryMatchStr(ctx, "[")) {
            sym* Symbol = Node->symbol;

            if (tokenTryMatchStr(ctx, "]"))
                Node = astCreateIndex(loc, Node, astCreateInvalid(loc));

            else {
                Node = astCreateIndex(loc, Node, parserValue(ctx));
                tokenMatchStr(ctx, "]");
            }

            Node->symbol = Symbol;
        }
    }

    debugLeave();

    return Node;
}

/**
 * DeclFunction = "(" [ DeclParam [{ "," DeclParam }] ] ")"
 */
static ast* parserDeclFunction (parserCtx* ctx, bool inDecl, bool inParam, ast* atom) {
    (void) inParam;

    debugEnter("DeclFunction");

    tokenMatchStr(ctx, "(");

    ast* Node = astCreateCall(ctx->location, atom);
    /*Propogate the declared symbol up the chain*/
    Node->symbol = atom->symbol;

    bool isFnDef = false;
    sym* Scope = ctx->scope;

    if (inDecl) {
        isFnDef = atom->class == astLiteral;

        if (isFnDef)
            ctx->scope = atom->symbol;
    }

    if (!tokenIs(ctx, ")")) do {
        astAddChild(Node, parserDeclParam(ctx, isFnDef));
    } while (tokenTryMatchStr(ctx, ","));

    ctx->scope = Scope;

    tokenMatchStr(ctx, ")");

    debugLeave();

    return Node;
}

/**
 * TypeAtom = [ ( "(" DeclExpr ")" ) | Name ]
 *
 * This is where the inDecl/inParam stuff we've been keeping track of
 * comes into play. If outside a declaration, idents are not allowed.
 * Inside a declaration, an ident is *required* unless in the parameters,
 * where they are optional (even outside a prototype...).
 */
static ast* parserDeclAtom (parserCtx* ctx, bool inDecl, bool inParam) {
    debugEnter("DeclAtom");

    ast* Node = 0;

    if (tokenTryMatchStr(ctx, "(")) {
        Node = parserDeclExpr(ctx, inDecl, inParam);
        tokenMatchStr(ctx, ")");

    } else if (tokenIsIdent(ctx)) {
        if (inDecl || inParam) {
            Node = parserName(ctx);

            if (inDecl) {
                if (inParam)
                    Node->symbol = symCreateParam(ctx->scope, (char*) Node->literal);

                else
                    Node->symbol = symCreateId(ctx->scope, (char*) Node->literal);
            }

        } else {
            Node = astCreateInvalid(ctx->location);
            errorIdentOutsideDecl(ctx);
            tokenNext(ctx);
        }

    } else if (inDecl && !inParam) {
        Node = astCreateInvalid(ctx->location);
        errorExpected(ctx, "name");

    } else
        Node = astCreateEmpty(ctx->location);

    debugLeave();

    return Node;
}

/**
 * Name = <UnqualifiedIdent>
 */
static ast* parserName (parserCtx* ctx) {
    debugEnter("Name");

    ast* Node = 0;

    if (tokenIsIdent(ctx)) {
        sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

        if (!Symbol)
            Node = astCreateLiteralIdent(ctx->location, tokenDupMatch(ctx));

        else {
            errorDuplicateSym(ctx);
            Node = astCreateInvalid(ctx->location);
            Node->literal = "";
            tokenNext(ctx);
        }

    } else
        errorExpected(ctx, "name");

    debugLeave();

    return Node;
}
