#include "../inc/parser-decl.h"

#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/debug.h"
#include "../inc/error.h"

#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserField (parserCtx* ctx);
static ast* parserEnumField (parserCtx* ctx);
static ast* parserParam (parserCtx* ctx, bool inDecl);

static storageTag parserStorage (parserCtx* ctx);
static ast* parserDeclBasic (parserCtx* ctx);
static ast* parserStructOrUnion (parserCtx* ctx);
static ast* parserEnum (parserCtx* ctx);

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
 *                                            | Code# )
 *
 * DeclExpr is told to require identifiers, allow initiations and
 * create symbols.
 */
ast* parserDecl (parserCtx* ctx, bool module) {
    debugEnter("Decl");

    tokenLocation loc = ctx->location;
    storageTag storage = parserStorage(ctx);

    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));

    if (tokenTryMatchPunct(ctx, punctSemicolon))
        ;

    else {
        /*Grammatically, typedef is a storage class, but semantically
          a symbol tag.*/
        symTag symt = storage == storageTypedef ? symTypedef : symId;

        astAddChild(Node, parserDeclExpr(ctx, true, symt));
        Node->lastChild->symbol->storage = storage;

        if (tokenIsPunct(ctx, punctLBrace)) {
            loc = ctx->location;
            Node = astCreateFnImpl(loc, Node);
            Node->symbol = Node->l->firstChild->symbol;

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
            while (tokenTryMatchPunct(ctx, punctComma))
                astAddChild(Node, parserDeclExpr(ctx, true, symt));

            tokenMatchPunct(ctx, punctSemicolon);
        }
    }

    debugLeave();

    return Node;
}

/**
 * Field = DeclBasic [ DeclExpr# [{ "," DeclExpr# }] ] ";"
 */
static ast* parserField (parserCtx* ctx) {
    debugEnter("Field");

    tokenLocation loc = ctx->location;
    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));

    if (!tokenIsPunct(ctx, punctSemicolon)) do {
        astAddChild(Node, parserDeclExpr(ctx, true, symId));
    } while (tokenTryMatchPunct(ctx, punctComma));

    tokenMatchPunct(ctx, punctSemicolon);

    debugLeave();

    return Node;
}

/**
 * EnumField = Name# [ "=" AssignValue ]
 */
static ast* parserEnumField (parserCtx* ctx) {
    debugEnter("EnumField");

    ast* Node = parserName(ctx, true, symEnumConstant);

    if (tokenIsPunct(ctx, punctAssign)) {
        tokenLocation loc = ctx->location;
        char* o = tokenDupMatch(ctx);
        Node = astCreateBOP(loc, Node, o, parserAssignValue(ctx));
        Node->symbol = Node->l->symbol;
    }

    debugLeave();

    return Node;
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

    storageTag storage = tokenTryMatchKeyword(ctx, keywordStatic) ? storageStatic :
                         tokenTryMatchKeyword(ctx, keywordExtern) ? storageExtern :
                         tokenTryMatchKeyword(ctx, keywordTypedef) ? storageTypedef :
                         (tokenTryMatchKeyword(ctx, keywordAuto), storageAuto);

    debugLeave();

    return storage;
}

/**
 * DeclBasic = [ "const" ] <Ident> | StructUnion | Enum
 */
static ast* parserDeclBasic (parserCtx* ctx) {
    debugEnter("DeclBasic");

    ast* Node = 0;

    tokenLocation loc = ctx->location;
    bool isConst = tokenTryMatchKeyword(ctx, keywordConst);

    if (tokenIsKeyword(ctx, keywordStruct) || tokenIsKeyword(ctx, keywordUnion))
        Node = parserStructOrUnion(ctx);

    else if (tokenIsKeyword(ctx, keywordEnum))
        Node = parserEnum(ctx);

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

    if (isConst) {
        Node = astCreateConst(loc, Node);
        Node->symbol = Node->r->symbol;
    }

    debugLeave();

    return Node;
}

/**
 * StructOrUnion = "struct" | "union" Name# ^ ( "{" [{ Field }] "}" )
 *
 * Name is told to create a symbol of the tag indicated by the first
 * token.
 */
static struct ast* parserStructOrUnion (parserCtx* ctx) {
    debugEnter("StructOrUnion");

    tokenLocation loc = ctx->location;

    symTag tag =   tokenTryMatchKeyword(ctx, keywordStruct)
                 ? symStruct
                 : (tokenMatchKeyword(ctx, keywordUnion), symUnion);

    ast* name;

    /*Name*/

    if (tokenIsIdent(ctx))
        name = parserName(ctx, true, tag);

    /*Anonymous struct, will require a body*/
    else {
        name = astCreateEmpty(loc);
        name->literal = strdup("");
        name->symbol = symCreateNamed(tag, ctx->scope, "");
    }

    ast* Node = (tag == symStruct ? astCreateStruct : astCreateUnion)
                (loc, name);
    Node->symbol = Node->l->symbol;
    sym* OldScope = scopeSet(ctx, Node->symbol);

    /*Body*/
    if (Node->l->tag == astEmpty || tokenIsPunct(ctx, punctLBrace)) {
        /*Only error if not already errored for wrong tag*/
        if (Node->symbol->impl && Node->symbol->tag == tag)
            errorReimplementedSym(ctx, Node->symbol);

        else
            Node->symbol->impl = Node;

        tokenMatchPunct(ctx, punctLBrace);

        Node->symbol->complete = true;

        /*Eat fields*/
        while (!tokenIsPunct(ctx, punctRBrace))
            astAddChild(Node, parserField(ctx));

        tokenMatchPunct(ctx, punctRBrace);
    }

    ctx->scope = OldScope;

    debugLeave();

    return Node;
}

/**
 * Enum = "enum" Name# ^ ( "{" EnumField [{ "," EnumField }] "}" )
 */
static struct ast* parserEnum (parserCtx* ctx) {
    debugEnter("Enum");

    tokenLocation loc = ctx->location;
    tokenMatchKeyword(ctx, keywordEnum);

    /*Name*/

    ast* name;

    if (tokenIsIdent(ctx))
        name = parserName(ctx, true, symEnum);

    else {
        name = astCreateEmpty(loc);
        name->literal = strdup("");
        name->symbol = symCreateNamed(symEnum, ctx->scope, "");
    }

    ast* Node = astCreateEnum(loc, name);
    Node->symbol = Node->l->symbol;

    /*Body*/
    if (Node->l->tag == astEmpty || tokenIsPunct(ctx, punctLBrace)) {
        tokenMatchPunct(ctx, punctLBrace);

        Node->symbol->complete = true;

        if (!tokenIsPunct(ctx, punctRBrace)) do {
            astAddChild(Node, parserEnumField(ctx));
        } while (tokenTryMatchPunct(ctx, punctComma));

        tokenMatchPunct(ctx, punctRBrace);
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
static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclExpr");

    ast* Node = parserDeclUnary(ctx, inDecl, tag);

    if (tokenIsPunct(ctx, punctAssign)) {
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
 * DeclUnary = ( "*" | "const" DeclUnary ) | DeclObject
 */
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclUnary");

    ast* Node = 0;
    tokenLocation loc = ctx->location;

    if (tokenIsPunct(ctx, punctTimes)) {
        char* o = tokenDupMatch(ctx);
        Node = astCreateUOP(loc, o, parserDeclUnary(ctx, inDecl, tag));
        Node->symbol = Node->r->symbol;

    } else if (tokenTryMatchKeyword(ctx, keywordConst)) {
        Node = astCreateConst(ctx->location, parserDeclUnary(ctx, inDecl, tag));
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

    while (tokenIsPunct(ctx, punctLParen) || tokenIsPunct(ctx, punctLBracket)) {
        tokenLocation loc = ctx->location;

        /*Function*/
        if (tokenIsPunct(ctx, punctLParen))
            Node = parserDeclFunction(ctx, inDecl, tag, Node);

        /*Array*/
        else if (tokenTryMatchPunct(ctx, punctLBracket)) {
            sym* Symbol = Node->symbol;

            if (tokenTryMatchPunct(ctx, punctRBracket))
                Node = astCreateIndex(loc, Node, astCreateEmpty(loc));

            else {
                Node = astCreateIndex(loc, Node, parserValue(ctx));
                tokenMatchPunct(ctx, punctRBracket);
            }

            Node->symbol = Symbol;
        }
    }

    debugLeave();

    return Node;
}

/**
 * DeclFunction = "(" [ ( Param [{ "," Param }] [ "," "..." ] ) | "..." ] ")"
 */
static ast* parserDeclFunction (parserCtx* ctx, bool inDecl, symTag tag, ast* atom) {
    (void) tag;

    debugEnter("DeclFunction");

    tokenMatchPunct(ctx, punctLParen);

    ast* Node = astCreateCall(ctx->location, atom);
    /*Propogate the declared symbol up the chain*/
    Node->symbol = atom->symbol;

    sym* OldScope = scopeSet(ctx, atom->symbol);

    if (!tokenIsPunct(ctx, punctRParen)) do {
        if (tokenIsPunct(ctx, punctEllipsis)) {
            astAddChild(Node, astCreateEllipsis(ctx->location));
            tokenMatch(ctx);
            break;

        } else
            astAddChild(Node, parserParam(ctx, inDecl));

    } while (tokenTryMatchPunct(ctx, punctComma));

    ctx->scope = OldScope;

    tokenMatchPunct(ctx, punctRParen);

    debugLeave();

    return Node;
}

/**
 * DeclAtom = [ ( "(" DeclExpr ")" ) | Name ]
 *
 * This is where the inDecl/tag stuff we've been keeping track of
 * comes into play. If outside a declaration, idents are not allowed.
 * Inside a declaration, an ident is *required* unless in the parameters,
 * where they are optional (even outside a prototype...).
 */
static ast* parserDeclAtom (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclAtom");

    ast* Node = 0;

    if (tokenTryMatchPunct(ctx, punctLParen)) {
        Node = parserDeclExpr(ctx, inDecl, tag);
        tokenMatchPunct(ctx, punctRParen);

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
        sym* Symbol = symFind(ctx->scope, (char*) Node->literal);

        if (Symbol) {
            Node->symbol = Symbol;

            /*SPECIAL EXCEPTION
              In C, there are multiple symbol tables for variables, typedefs, structs etc
              But not in fcc! So there is this special exception: structs and unions and
              enums may be redeclared as typedefs, to allow for this idiom:

              typedef struct x {
                  ...
              } x;

              Doesn't guarantee that it's redeclaring the *right* symbol.*/
            if (   Node->symbol->tag != tag
                && !(   (   Node->symbol->tag == symStruct
                         || Node->symbol->tag == symUnion
                         || Node->symbol->tag == symEnum)
                     && tag == symTypedef))
                errorRedeclaredSymAs(ctx, Node->symbol, tag);

        } else if (inDecl)
            Node->symbol = symCreateNamed(tag, ctx->scope, (char*) Node->literal);

        /*Can't tell whether this is a duplicate declaration
          or a (matching) redefinition*/
        vectorPush(&Node->symbol->decls, Node);

    } else {
        errorExpected(ctx, "name");
        Node = astCreateInvalid(ctx->location);
        Node->literal = strdup("");
        Node->symbol = symCreateNamed(tag, ctx->scope, "");
    }

    debugLeave();

    return Node;
}
