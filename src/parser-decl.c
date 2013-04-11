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
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, bool inParam);
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, bool inParam);
static ast* parserDeclFunction (parserCtx* ctx, bool inDecl, bool inParam, ast* atom);
static ast* parserDeclAtom (parserCtx* ctx, bool inDecl, bool inParam);

static ast* parserName (parserCtx* ctx);

/**
 * DeclStruct = "struct" Name [ "{" [{ DeclField ";" }] "}" ] ";"
 */
struct ast* parserDeclStruct (parserCtx* ctx) {
    debugEnter("Struct");

    tokenMatchStr(ctx, "struct");

    ast* Node = astCreateDeclStruct(ctx->location, parserName(ctx));
    Node->symbol = symCreateStruct(ctx->scope, (char*) Node->l->literal);

    sym* OldScope = scopeSet(ctx, Node->symbol);

    /*Body?*/
    if (tokenTryMatchStr(ctx, "{")) {
        if (Node->symbol->def)
            errorRedefinedSym(ctx, Node->symbol);

        else
            Node->symbol->def = Node;

        /*Eat fields*/
        while (!tokenIs(ctx, "}")) {
            astAddChild(Node, parserDeclField(ctx));
            tokenMatchStr(ctx, ";");
        }

        tokenMatchStr(ctx, "}");
    }

    tokenMatchStr(ctx, ";");

    ctx->scope = OldScope;

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
 * ModuleDecl = DeclBasic DeclExpr#   ( "{" Code "}" )
 *                                  | ( [{ "," DeclExpr# }] ";" )
 */
ast* parserModuleDecl (parserCtx* ctx) {
    debugEnter("ModuleDecl");

    tokenLocation loc = ctx->location;
    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));

    astAddChild(Node, parserDeclExpr(ctx, true, false));

    if (tokenIs(ctx, "{")) {
        loc = ctx->location;
        Node = astCreateFnImpl(loc, Node);
        Node->symbol = Node->l->firstChild->symbol;

        if (Node->symbol->def)
            errorRedefinedSym(ctx, Node->symbol);

        else
            Node->symbol->def = Node;

        sym* OldScope = scopeSet(ctx, Node->symbol);
        Node->r = parserCode(ctx);
        ctx->scope = OldScope;

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
    Node->symbol = Node->r->symbol;

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
 * DeclExpr = DeclUnary [ "=" AssignValue ]
 *
 * This uses AssignValue instead of Value, skipping comma operators.
 * This is to avoid the case of
 *     int x = 5, y = 6;
 * which would be parsed as
 *     int x = (5, y = 6);
 */
static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, bool inParam) {
    debugEnter("DeclExpr");

    ast* Node = parserDeclUnary(ctx, inDecl, inParam);

    if (tokenIs(ctx, "=")) {
        if (inDecl && !inParam) {
            tokenLocation loc = ctx->location;
            char* o = tokenDupMatch(ctx);
            Node = astCreateBOP(loc, Node, o, parserAssignValue(ctx));
            Node->symbol = Node->l->symbol;

            if (Node->symbol->def)
                errorRedefinedSym(ctx, Node->symbol);

            else
                Node->symbol->def = Node;

        } else {
            errorIllegalOutside(ctx, "initializer", "a declaration");
            Node = astCreateInvalid(ctx->location);
        }
    }

    debugLeave();

    return Node;
}

/**
 * DeclUnary = ( "*" DeclUnary ) | DeclObject
 */
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, bool inParam) {
    debugEnter("DeclUnary");

    ast* Node = 0;

    if (tokenIs(ctx, "*")) {
        tokenLocation loc = ctx->location;
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(loc, o, parserDeclUnary(ctx, inDecl, inParam));
        Node->symbol = Node->r->symbol;

    } else
        Node = parserDeclObject(ctx, inDecl, inParam);

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
                Node = astCreateIndex(loc, Node, astCreateEmpty(loc));

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

    sym* OldScope = scopeSet(ctx, atom->symbol);

    if (!tokenIs(ctx, ")")) do {
        astAddChild(Node, parserDeclParam(ctx, inDecl));
    } while (tokenTryMatchStr(ctx, ","));

    ctx->scope = OldScope;

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

            /*Check for a collision only in this scope, will shadow any
              other declarations elsewhere.*/
            sym* Symbol = symChild(ctx->scope, (char*) Node->literal);

            if (Symbol) {
                /*Can't tell whether this is a duplicate declaration
                  or a (matching) redefinition*/
                vectorAdd(&Symbol->decls, Node);
                Node->symbol = Symbol;

            } else {
                if (inDecl) {
                    if (inParam)
                        Node->symbol = symCreateParam(ctx->scope, (char*) Node->literal);

                    else
                        Node->symbol = symCreateId(ctx->scope, (char*) Node->literal);

                    vectorAdd(&Node->symbol->decls, Node);
                }
            }

        } else {
            Node = astCreateInvalid(ctx->location);
            errorIllegalOutside(ctx, "identifier", "a declaration");
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

    if (tokenIsIdent(ctx))
        Node = astCreateLiteralIdent(ctx->location, tokenDupMatch(ctx));

    else {
        errorExpected(ctx, "name");
        Node = astCreateInvalid(ctx->location);
        Node->literal = strdup("");
    }

    debugLeave();

    return Node;
}
