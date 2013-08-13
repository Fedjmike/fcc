#include "../inc/parser-decl.h"

#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/debug.h"

#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserField (parserCtx* ctx);
static ast* parserParam (parserCtx* ctx, bool inDecl);

static storageTag parserStorage (parserCtx* ctx);
static ast* parserDeclBasic (parserCtx* ctx);
static ast* parserStructOrUnion (parserCtx* ctx);

static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, symTag tag);
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, symTag tag);
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, symTag tag);
static ast* parserDeclFunction (parserCtx* ctx, bool inDecl, symTag tag, ast* atom);
static ast* parserDeclAtom (parserCtx* ctx, bool inDecl, symTag tag);
static ast* parserName (parserCtx* ctx, bool inDecl, symTag tag);

/**
 * Type = DeclBasic DeclExpr#
 *
 * DeclExpr is told not to allow identifiers or initiations, and not
 * to create symbols.
 */
 ast* parserType (parserCtx* ctx) {
    debugEnter("Type");

    tokenLocation loc = ctx->location;
    ast* basic = parserDeclBasic(ctx);
    ast* expr = parserDeclExpr(ctx, false, symUndefined);
    ast* Node = astCreateType(loc, basic, expr);

    debugLeave();

    return Node;
}

/**
 * Decl = Storage DeclBasic ";" | ( DeclExpr#   ( [{ "," DeclExpr# }] ";" )
 *                                            | ( "{" Code "}" ) )
 *
 * DeclExpr is told to require identifiers, allow initiations and
 * create symbols.
 */
ast* parserDecl (parserCtx* ctx, bool module) {
    debugEnter("Decl");

    tokenLocation loc = ctx->location;
    storageTag storage = parserStorage(ctx);
    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));

    if (tokenTryMatchStr(ctx, ";"))
        ;

    else {
        /*Grammatically, typedef is a storage class, but semantically
          a symbol tag.*/
        astAddChild(Node, parserDeclExpr(ctx, true,   storage == storageTypedef
                                                    ? symTypedef
                                                    : symId));

        if (tokenIs(ctx, "{")) {
            loc = ctx->location;
            Node = astCreateFnImpl(loc, Node);
            Node->symbol = Node->l->firstChild->symbol;
            Node->symbol->storage = storage;

            if (Node->symbol->impl)
                errorReimplementedSym(ctx, Node->symbol);

            else
                Node->symbol->impl = Node;

            if (!module)
                errorIllegalOutside(ctx, "function implementation", "module level code");

            sym* OldScope = scopeSet(ctx, Node->symbol);
            Node->r = parserCode(ctx);
            ctx->scope = OldScope;

        } else {
            while (tokenTryMatchStr(ctx, ","))
                astAddChild(Node, parserDeclExpr(ctx, true, symId));

            tokenMatchStr(ctx, ";");
        }
    }

    debugLeave();

    return Node;
}

/**
 * Field = Decl#
 */
static ast* parserField (parserCtx* ctx) {
    return parserDecl(ctx, false);
    /*!!!CHANGEZ MOI!!!
      To what?
      Differences: not allowed to assign values, not allowed to decl
                   functions, certainly not implement them
      Decling fns is an issue for the analyzer*/
}

/**
 * Param = DeclBasic DeclExpr#
 *
 * DeclExpr is told to accept but not require identifiers and symbol
 * creation if and only if inDecl is true.
 */
static ast* parserParam (parserCtx* ctx, bool inDecl) {
    debugEnter("Param");

    tokenLocation loc = ctx->location;
    ast* basic = parserDeclBasic(ctx);
    ast* expr = parserDeclExpr(ctx, inDecl, symParam);
    ast* Node = astCreateParam(loc, basic, expr);
    Node->symbol = Node->r->symbol;

    debugLeave();

    return Node;
}

/**
 * Storage = [ "auto" | "static" | "extern" | "typedef" ]
 */
static storageTag parserStorage (parserCtx* ctx) {
    debugEnter("Storage");

    storageTag storage = storageAuto;

    if (tokenIs(ctx, "auto")) {
        storage = storageAuto;
        tokenMatch(ctx);

    } else if (tokenIs(ctx, "static")) {
        storage = storageStatic;
        tokenMatch(ctx);

    } else if (tokenIs(ctx, "extern")) {
        storage = storageExtern;
        tokenMatch(ctx);

    } else if (tokenIs(ctx, "typedef")) {
        storage = storageTypedef;
        tokenMatch(ctx);

    } else
        ;

    debugLeave();

    return storage;
}

/**
 * DeclBasic = <Ident> | StructUnion
 */
static ast* parserDeclBasic (parserCtx* ctx) {
    debugEnter("DeclBasic");

    ast* Node = 0;

    if (tokenIs(ctx, "struct") || tokenIs(ctx, "union"))
        Node = parserStructOrUnion(ctx);

    else {
        sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

        if (Symbol) {
            Node = astCreateLiteralIdent(ctx->location, tokenDupMatch(ctx));
            Node->symbol = Symbol;

        } else {
            if (tokenIsIdent(ctx)) {
                errorUndefType(ctx);
                tokenNext(ctx);

            } else
                errorExpected(ctx, "type name");

            Node = astCreateInvalid(ctx->location);
        }
    }

    debugLeave();

    return Node;
}

/**
 * StructOrUnion = "struct" | "union" [ Name# ]
 *                 [ "{" [{ Field }] "}" ]
 *
 * Name is told to create a symbol of the tag indicated by the first
 * token.
 */
static struct ast* parserStructOrUnion (parserCtx* ctx) {
    debugEnter("StructOrUnion");

    symTag tag =   tokenTryMatchStr(ctx, "struct")
                 ? symStruct
                 : (tokenMatchStr(ctx, "union"), symUnion);

    ast* Node = (tag == symStruct ? astCreateStruct : astCreateUnion)
                (ctx->location, parserName(ctx, true, tag));
    Node->symbol = Node->l->symbol;
    sym* OldScope = scopeSet(ctx, Node->symbol);

    /*Body?*/
    if (tokenTryMatchStr(ctx, "{")) {
        /*Only error if not already errored for wrong tag*/
        if (Node->symbol->impl && Node->symbol->tag == tag)
            errorReimplementedSym(ctx, Node->symbol);

        else
            Node->symbol->impl = Node;

        /*Eat fields*/
        while (!tokenIs(ctx, "}"))
            astAddChild(Node, parserField(ctx));

        tokenMatchStr(ctx, "}");
    }

    ctx->scope = OldScope;

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
static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclExpr");

    ast* Node = parserDeclUnary(ctx, inDecl, tag);

    if (tokenIs(ctx, "=")) {
        if (inDecl && tag == symId) {
            tokenLocation loc = ctx->location;
            char* o = tokenDupMatch(ctx);
            Node = astCreateBOP(loc, Node, o, parserAssignValue(ctx));
            Node->symbol = Node->l->symbol;

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
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclUnary");

    ast* Node = 0;

    if (tokenIs(ctx, "*")) {
        tokenLocation loc = ctx->location;
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(loc, o, parserDeclUnary(ctx, inDecl, tag));
        Node->symbol = Node->r->symbol;

    } else
        Node = parserDeclObject(ctx, inDecl, tag);

    debugLeave();

    return Node;
}

/**
 * DeclObject = DeclAtom [{ DeclFunction | ( "[" Value "]" ) }]
 */
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclObject");

    ast* Node = parserDeclAtom(ctx, inDecl, tag);

    while (tokenIs(ctx, "(") || tokenIs(ctx, "[")) {
        tokenLocation loc = ctx->location;

        /*Function*/
        if (tokenIs(ctx, "("))
            Node = parserDeclFunction(ctx, inDecl, tag, Node);

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
 * DeclFunction = "(" [ ( Param [{ "," Param }] [ "," "..." ] ) | "..." ]  ")"
 */
static ast* parserDeclFunction (parserCtx* ctx, bool inDecl, symTag tag, ast* atom) {
    (void) tag;

    debugEnter("DeclFunction");

    tokenMatchStr(ctx, "(");

    ast* Node = astCreateCall(ctx->location, atom);
    /*Propogate the declared symbol up the chain*/
    Node->symbol = atom->symbol;

    sym* OldScope = scopeSet(ctx, atom->symbol);

    if (!tokenIs(ctx, ")")) do {
        if (tokenIs(ctx, "...")) {
            astAddChild(Node, astCreateEllipsis(ctx->location));
            tokenMatch(ctx);
            break;

        } else
            astAddChild(Node, parserParam(ctx, inDecl));

    } while (tokenTryMatchStr(ctx, ","));

    ctx->scope = OldScope;

    tokenMatchStr(ctx, ")");

    debugLeave();

    return Node;
}

/**
 * TypeAtom = [ ( "(" DeclExpr ")" ) | Name ]
 *
 * This is where the inDecl/tag stuff we've been keeping track of
 * comes into play. If outside a declaration, idents are not allowed.
 * Inside a declaration, an ident is *required* unless in the parameters,
 * where they are optional (even outside a prototype...).
 */
static ast* parserDeclAtom (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclAtom");

    ast* Node = 0;

    if (tokenTryMatchStr(ctx, "(")) {
        Node = parserDeclExpr(ctx, inDecl, tag);
        tokenMatchStr(ctx, ")");

    } else if (tokenIsIdent(ctx)) {
        if (inDecl || tag == symParam)
            Node = parserName(ctx, inDecl, tag);

        else {
            Node = astCreateInvalid(ctx->location);
            errorIllegalOutside(ctx, "identifier", "a declaration");
            tokenNext(ctx);
        }

    } else if (inDecl && tag != symParam)
        Node = parserName(ctx, inDecl, tag);

    else
        Node = astCreateEmpty(ctx->location);

    debugLeave();

    return Node;
}

/**
 * Name = <UnqualifiedIdent>
 *
 * If inDecl, creates a symbol, adding it to the list of declarations
 * if already created.
 */
static ast* parserName (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("Name");

    ast* Node = 0;

    if (tokenIsIdent(ctx)) {
        Node = astCreateLiteralIdent(ctx->location, tokenDupMatch(ctx));

        /*Check for a collision only in this scope, will shadow any
          other declarations elsewhere.*/
        sym* Symbol = symChild(ctx->scope, (char*) Node->literal);

        if (Symbol) {
            Node->symbol = Symbol;

            /*SPECIAL EXCEPTION
              In C, there are multiple symbol tables for variables, typedefs, structs etc
              But not in fcc! So there is this special exception: structs and unions may
              be redeclared as typedefs, to allow for this idiom:

              typedef struct x {
                  ...
              } x;

              Doesn't guarantee that it's redeclaring the *right* symbol.*/
            if (   Node->symbol->tag != tag
                && !(   (   Node->symbol->tag == symStruct
                         || Node->symbol->tag == symUnion)
                     && tag == symTypedef))
                errorRedeclaredSymAs(ctx, Node->symbol, tag);

        } else if (inDecl)
            Node->symbol = symCreateNamed(tag, ctx->scope, (char*) Node->literal);

        /*Can't tell whether this is a duplicate declaration
          or a (matching) redefinition*/
        vectorAdd(&Node->symbol->decls, Node);

    } else {
        errorExpected(ctx, "name");
        Node = astCreateInvalid(ctx->location);
        Node->literal = strdup("");
        Node->symbol = symCreateNamed(tag, ctx->scope, "");
    }

    debugLeave();

    return Node;
}
