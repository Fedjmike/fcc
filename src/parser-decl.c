#include "../inc/parser-decl.h"

#include "../inc/debug.h"
#include "../inc/sym.h"
#include "../inc/ast.h"
#include "../inc/error.h"

#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

#include "stdlib.h"

using "../inc/debug.h";
using "../inc/sym.h";
using "../inc/ast.h";
using "../inc/error.h";

using "../inc/lexer.h";
using "../inc/parser-helpers.h";
using "../inc/parser-value.h";

using "stdlib.h";
using "string.h";

static ast* parserStorage (parserCtx* ctx, symTag* tag);
static ast* parserFnImpl (parserCtx* ctx, ast* decl);

static ast* parserField (parserCtx* ctx);
static ast* parserEnumField (parserCtx* ctx);
static ast* parserParam (parserCtx* ctx, bool inDecl);

static ast* parserDeclBasic (parserCtx* ctx);
static ast* parserStructOrUnion (parserCtx* ctx);
static ast* parserEnum (parserCtx* ctx);

static ast* parserDeclExpr (parserCtx* ctx, bool inDecl, symTag tag);
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, symTag tag);
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, symTag tag);
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
 * In C, prototypes are allowed to name the parameters but these names aren't
 * actually used; only the declarations of the parameters from the function
 * implementation matter. However, during the parsing of a decl we can't tell
 * whether we're seeing an implementation or not!
 * So, we call this function after the '{' is seen and it traces up the AST
 * creating param symbols. @see parserFnImpl
 * To complicate matter, for diagnostics purposes we *do* actually create param
 * symbols for prototypes, but we put them in a "bin". @see parserDeclObject
 */
static void parserCreateParamSymbols (ast* Node, sym* scope) {
    /*Trace up the AST to find the astCall*/

    ast* call = Node;

    while (call->tag != astCall) {
        if (!call)
            return;

        else if (call->tag == astBOP || call->tag == astIndex)
            call = call->l;

        else if (call->tag == astUOP || call->tag == astConst)
            call = call->r;

        else
            return;
    }

    /*For each param*/
    for (ast* param = call->firstChild;
         param;
         param = param->nextSibling) {
        /*Trace up to find the identifier*/

        ast* ident = param;

        while (ident->tag != astLiteral) {
            if (!ident)
                break;

            else if (   ident->tag == astBOP || ident->tag == astIndex
                     || ident->tag == astCall)
                ident = ident->l;

            else if (   ident->tag == astUOP || ident->tag == astConst
                     || ident->tag == astParam)
                ident = ident->r;

            else {
                ident = 0;
                break;
            }
        }

        /*Found the ident, create the symbol*/
        if (ident && ident->litTag == literalIdent)
            param->symbol = symCreateNamed(symParam, scope, (char*) ident->literal);

        /*Trace up again, assigning the symbol to all the nodes*/
        for (ident = param; ident;) {
            ident->symbol = param->symbol;

            if (   ident->tag == astBOP || ident->tag == astIndex
                || ident->tag == astCall)
                ident = ident->l;

            else if (   ident->tag == astUOP || ident->tag == astConst
                     || ident->tag == astParam)
                ident = ident->r;

            else
                break;
        }
    }
}

/**
 * Decl = Storage DeclBasic ";" | ( DeclExpr#   ( [{ "," DeclExpr# }] ";" )
 *                                            | FnImpl )
 *
 * DeclExpr is told to require identifiers, allow initiations and
 * create symbols.
 */
ast* parserDecl (parserCtx* ctx, bool module) {
    debugEnter("Decl");

    tokenLocation loc = ctx->location;

    symTag tag = symId;
    ast* storage = parserStorage(ctx, &tag);
    ast* Node = astCreateDecl(loc, parserDeclBasic(ctx));
    Node->r = storage;

    /*Declares no symbols*/
    if (tokenTryMatchPunct(ctx, punctSemicolon))
        ;

    else {
        astAddChild(Node, parserDeclExpr(ctx, true, tag));

        /*Function*/
        if (tokenIsPunct(ctx, punctLBrace)) {
            if (!module)
                errorIllegalOutside(ctx, "function implementation", "module level code");

            Node = parserFnImpl(ctx, Node);

        /*Regular decl*/
        } else {
            while (tokenTryMatchPunct(ctx, punctComma))
                astAddChild(Node, parserDeclExpr(ctx, true, tag));

            tokenMatchPunct(ctx, punctSemicolon);
        }
    }

    debugLeave();

    return Node;
}

/**
 * Storage = [ "auto" | "static" | "extern" | "typedef" ]
 */
static ast* parserStorage (parserCtx* ctx, symTag* tag) {
    debugEnter("Storage");

    ast* Node = 0;

    markerTag marker =   tokenIsKeyword(ctx, keywordAuto) ? markerAuto
                       : tokenIsKeyword(ctx, keywordStatic) ? markerStatic
                       : tokenIsKeyword(ctx, keywordExtern) ? markerExtern : markerUndefined;

    if (tokenTryMatchKeyword(ctx, keywordTypedef))
        *tag = symTypedef;

    else if (marker) {
        Node = astCreateMarker(ctx->location, marker);
        tokenMatch(ctx);
    }

    debugLeave();

    return Node;
}

/**
 * FnImpl = Code
 */
static ast* parserFnImpl (parserCtx* ctx, ast* decl) {
    debugEnter("FnImpl");

    sym* fn = decl->firstChild->symbol;

    ast* Node = astCreateFnImpl(ctx->location, decl);
    Node->symbol = fn;

    /*Is a function implementation valid for this symbol?*/

    if (fn->impl)
        errorReimplementedSym(ctx, fn);

    else {
        fn->impl = Node;
        /*Now that we have the implementation, create param symbols*/
        parserCreateParamSymbols(decl->firstChild, fn);
    }

    /*Body*/

    sym* OldScope = scopeSet(ctx, fn);
    Node->r = parserCode(ctx);
    ctx->scope = OldScope;

    debugLeave();

    return Node;
}

/**
 * Field = DeclBasic [ DeclExpr# [{ "," DeclExpr# }] ] ";"
 *
 * DeclExpr is told to require idents and create symbols.
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
    tokenLocation loc = ctx->location;

    if (tokenTryMatchPunct(ctx, punctAssign)) {
        Node = astCreateBOP(loc, Node, opAssign, parserAssignValue(ctx));
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
 * DeclBasic = [ "const" ] <Ident> | StructUnion | Enum
 */
static ast* parserDeclBasic (parserCtx* ctx) {
    debugEnter("DeclBasic");

    ast* Node;

    tokenLocation constloc = ctx->location;
    bool isConst = tokenTryMatchKeyword(ctx, keywordConst);

    if (tokenIsKeyword(ctx, keywordStruct) || tokenIsKeyword(ctx, keywordUnion))
        Node = parserStructOrUnion(ctx);

    else if (tokenIsKeyword(ctx, keywordEnum))
        Node = parserEnum(ctx);

    else {
        tokenLocation loc = ctx->location;
        sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

        if (Symbol) {
            Node = astCreateLiteralIdent(loc, tokenDupMatch(ctx));
            Node->symbol = Symbol;

        } else {
            if (tokenIsIdent(ctx)) {
                errorUndefType(ctx);
                tokenNext(ctx);

            } else
                errorExpected(ctx, "type name");

            Node = astCreateInvalid(loc);
        }
    }

    if (isConst) {
        Node = astCreateConst(constloc, Node);
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
static ast* parserStructOrUnion (parserCtx* ctx) {
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
 * Enum = "enum" Name# ^ ( "{" EnumField [{ "," EnumField }] [ "," ] "}" )
 */
static ast* parserEnum (parserCtx* ctx) {
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
    Node->symbol->complete = true;

    /*Body*/
    if (Node->l->tag == astEmpty || tokenIsPunct(ctx, punctLBrace)) {
        /*Only error if not already errored for wrong tag*/
        if (Node->symbol->impl && Node->symbol->tag == symEnum)
            errorReimplementedSym(ctx, Node->symbol);

        else
            Node->symbol->impl = Node;

        tokenMatchPunct(ctx, punctLBrace);

        if (!tokenIsPunct(ctx, punctRBrace)) do {
            astAddChild(Node, parserEnumField(ctx));
        } while (tokenTryMatchPunct(ctx, punctComma) && !tokenIsPunct(ctx, punctRBrace));

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
    tokenLocation loc = ctx->location;

    if (tokenTryMatchPunct(ctx, punctAssign)) {
        if (!inDecl || tag != symId)
            errorIllegalOutside(ctx, "initializer", "a declaration");

        Node = astCreateBOP(loc, Node, opAssign, parserAssignValue(ctx));
        Node->symbol = Node->l->symbol;
        Node->symbol->impl = Node->r;
    }

    debugLeave();

    return Node;
}

/**
 * DeclUnary = ( "*" | "const" DeclUnary ) | DeclObject
 */
static ast* parserDeclUnary (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclUnary");

    ast* Node;
    tokenLocation loc = ctx->location;

    if (tokenTryMatchPunct(ctx, punctTimes)) {
        Node = astCreateUOP(loc, opDeref, parserDeclUnary(ctx, inDecl, tag));
        Node->symbol = Node->r->symbol;

    } else if (tokenTryMatchKeyword(ctx, keywordConst)) {
        Node = astCreateConst(loc, parserDeclUnary(ctx, inDecl, tag));
        Node->symbol = Node->r->symbol;

    } else
        Node = parserDeclObject(ctx, inDecl, tag);

    debugLeave();

    return Node;
}

/**
 * DeclObject = DeclAtom [{ ParamList | ( "[" Value "]" ) }]
 */
static ast* parserDeclObject (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("DeclObject");

    tokenLocation loc = ctx->location;
    ast* Node = parserDeclAtom(ctx, inDecl, tag);
    sym* Symbol = Node->symbol;

    while (true) {
        /*Function*/
        if (tokenIsPunct(ctx, punctLParen)) {
            /*The proper param symbols are created just before the parsing of the body,
              prototype params go in the bin, but exist for diagnostics*/
            sym* OldScope = scopeSet(ctx, symCreateScope(ctx->scope));

            Node = astCreateCall(loc, Node);
            parserParamList(ctx, Node, inDecl);

            ctx->scope = OldScope;

        /*Array*/
        } else if (tokenTryMatchPunct(ctx, punctLBracket)) {
            if (tokenIsPunct(ctx, punctRBracket))
                /*An empty [] is a synonym for * when in params*/
                Node =   tag == symParam
                       ? astCreateUOP(loc, opDeref, Node)
                       : astCreateIndex(loc, Node, astCreateEmpty(ctx->location));

            else
                Node = astCreateIndex(loc, Node, parserValue(ctx));

            tokenMatchPunct(ctx, punctRBracket);

        } else
            break;

        /*Propogate the declared symbol up the chain*/
        Node->symbol = Symbol;
    }

    debugLeave();

    return Node;
}

/**
 * ParamList = "(" [ ( Param [{ "," Param }] [ "," "..." ] ) | "..." | "void" ] ")"
 */
void parserParamList (parserCtx* ctx, ast* Node, bool inDecl) {
    debugEnter("ParamList");

    tokenMatchPunct(ctx, punctLParen);

    if (!tokenIsPunct(ctx, punctRParen)) {
        tokenLocation voidloc = ctx->location;

        bool end = false;

        /*Either an empty prototype declaration,
          or the beginning of a void* parameter*/
        if (tokenTryMatchKeyword(ctx, keywordVoid)) {
            if (!tokenIsPunct(ctx, punctRParen)) {
                ast* basic = astCreateLiteralIdent(voidloc, strdup("void"));
                ast* expr = parserDeclExpr(ctx, inDecl, symParam);
                ast* param = astCreateParam(voidloc, basic, expr);
                param->symbol = basic->symbol = symFind(ctx->scope, "void");
                astAddChild(Node, param);

                if (!tokenTryMatchPunct(ctx, punctComma))
                    end = true;

            } else
                end = true;
        }

        if (!end) do {
            if (tokenIsPunct(ctx, punctEllipsis)) {
                astAddChild(Node, astCreate(astEllipsis, ctx->location));
                tokenMatch(ctx);
                break;

            } else
                astAddChild(Node, parserParam(ctx, inDecl));

        } while (tokenTryMatchPunct(ctx, punctComma));
    }

    tokenMatchPunct(ctx, punctRParen);

    debugLeave();
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

    ast* Node;

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

static bool isTypedefException (sym* Symbol, symTag newtag) {
    /*SPECIAL EXCEPTION
      In C, there are multiple symbol tables for variables, typedefs, structs etc
      But not in fcc! So there is this special exception: structs and unions and
      enums may be redeclared as typedefs, to allow for this idiom:

      typedef struct x {
          ...
      } x;

      Doesn't guarantee that it's redeclaring the *right* symbol.*/
    return    newtag == symTypedef
           && (   Symbol->tag == symStruct
               || Symbol->tag == symUnion
               || Symbol->tag == symEnum);
}

/**
 * Name = <UnqualifiedIdent>
 *
 * If inDecl, creates a symbol, adding it to the list of declarations
 * if already created.
 */
static ast* parserName (parserCtx* ctx, bool inDecl, symTag tag) {
    debugEnter("Name");

    ast* Node;

    if (tokenIsIdent(ctx)) {
        tokenLocation loc = ctx->location;
        Node = astCreateLiteralIdent(loc, tokenDupMatch(ctx));

        /*Check for a collision only in this scope, will shadow any
          other declarations elsewhere.*/
        sym* Symbol = symFind(ctx->scope, (char*) Node->literal);

        if (Symbol) {
            Node->symbol = Symbol;
            symChangeParent(Symbol, ctx->scope);

            if (Node->symbol->tag != tag && !isTypedefException(Node->symbol, tag))
                errorRedeclaredSymAs(ctx, Node->symbol, tag);

        } else if (inDecl)
            Node->symbol = symCreateNamed(tag, ctx->scope, (char*) Node->literal);

        if (Node->symbol) {
            /*Can't tell whether this is a duplicate declaration
              or a (matching) redefinition*/
            vectorPush(&Node->symbol->decls, Node);
        }

    } else {
        if (ctx->lexer->token == tokenKeyword) {
            errorKeywordAsIdent(ctx);
            tokenNext(ctx);

        } else
            errorExpected(ctx, "identifier");

        Node = astCreateInvalid(ctx->location);
        Node->literal = strdup("");
        Node->symbol = symCreateNamed(tag, ctx->scope, "");
    }

    debugLeave();

    return Node;
}
