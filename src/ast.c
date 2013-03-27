#include "../inc/ast.h"

#include "../inc/debug.h"
#include "../inc/type.h"

#include "stdio.h"
#include "stdlib.h"

ast* astCreate (astClass class, tokenLocation location) {
    ast* Node = malloc(sizeof(ast));
    Node->class = class;

    Node->location = location;

    Node->firstChild = 0;
    Node->lastChild = 0;
    Node->nextSibling = 0;
    Node->prevSibling = 0;
    Node->children = 0;

    Node->l = 0;
    Node->o = 0;
    Node->r = 0;
    Node->dt = 0;

    Node->symbol = 0;

    Node->litClass = literalUndefined;
    Node->literal = 0;

    return Node;
}

ast* astCreateInvalid (tokenLocation location) {
    return astCreate(astInvalid, location);
}

ast* astCreateEmpty (tokenLocation location) {
    return astCreate(astEmpty, location);
}

ast* astCreateFnImpl (tokenLocation location, ast* decl, ast* impl) {
    ast* Node = astCreate(astFnImpl, location);
    Node->l = decl;
    Node->r = impl;
    return Node;
}

ast* astCreateDeclStruct (tokenLocation location, ast* name) {
    ast* Node = astCreate(astDeclStruct, location);
    Node->l = name;
    return Node;
}

ast* astCreateDecl (tokenLocation location, ast* basic) {
    ast* Node = astCreate(astDecl, location);
    Node->l = basic;
    return Node;
}

ast* astCreateDeclParam (tokenLocation location, ast* basic, ast* expr) {
    ast* Node = astCreate(astDeclParam, location);
    Node->l = basic;
    Node->r = expr;
    return Node;
}

ast* astCreateType (tokenLocation location, ast* basic, ast* expr) {
    ast* Node = astCreate(astType, location);
    Node->l = basic;
    Node->r = expr;
    return Node;
}

ast* astCreateBOP (tokenLocation location, ast* l, char* o, ast* r) {
    ast* Node = astCreate(astBOP, location);
    Node->l = l;
    Node->o = o;
    Node->r = r;
    return Node;
}

ast* astCreateUOP (tokenLocation location, char* o, ast* r) {
    ast* Node = astCreate(astUOP, location);
    Node->o = o;
    Node->r = r;
    return Node;
}

ast* astCreateTOP (tokenLocation location, ast* cond, ast* l, ast* r) {
    ast* Node = astCreate(astTOP, location);
    astAddChild(Node, cond);
    Node->l = l;
    Node->r = r;
    return Node;
}

ast* astCreateIndex (tokenLocation location, ast* base, ast* index) {
    ast* Node = astCreate(astIndex, location);
    Node->l = base;
    Node->r = index;
    return Node;
}

ast* astCreateCall (tokenLocation location, ast* function) {
    ast* Node = astCreate(astCall, location);
    Node->l = function;
    Node->symbol = Node->l->symbol;
    return Node;
}

ast* astCreateCast (tokenLocation location, ast* result) {
    ast* Node = astCreate(astCast, location);
    Node->l = result;
    return Node;
}

ast* astCreateLiteral (tokenLocation location, literalClass litClass) {
    ast* Node = astCreate(astLiteral, location);
    Node->litClass = litClass;
    return Node;
}

ast* astCreateLiteralIdent (tokenLocation location, char* ident) {
    ast* Node = astCreateLiteral(location, literalIdent);
    Node->literal = (void*) ident;
    return Node;
}

void astDestroy (ast* Node) {
    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling)
        astDestroy(Current);

    if (Node->l)
        astDestroy(Node->l);

    if (Node->r)
        astDestroy(Node->r);

    if (Node->dt)
        typeDestroy(Node->dt);

    free(Node->o);
    free(Node->literal);
    free(Node);
}

void astAddChild (ast* Parent, ast* Child) {
    if (!Child || !Parent) {
        printf("astAddChild(): null %s given.\n",
               !Parent ? "parent" : "child");
        return;
    }

    if (Parent->firstChild == 0) {
        Parent->firstChild = Child;
        Parent->lastChild = Child;

    } else {
        Child->prevSibling = Parent->lastChild;
        Parent->lastChild->nextSibling = Child;
        Parent->lastChild = Child;
    }

    Parent->children++;
}

int astIsValueClass (astClass class) {
    return    class == astBOP || class == astUOP || class == astTOP
           || class == astCall || class == astIndex || class == astCast
           || class == astLiteral;
}

const char* astClassGetStr (astClass class) {
    if (class == astUndefined)
        return "astUndefined";
    else if (class == astInvalid)
        return "astInvalid";
    else if (class == astEmpty)
        return "astEmpty";
    else if (class == astModule)
        return "astModule";
    else if (class == astFnImpl)
        return "astFnImpl";
    else if (class == astDeclStruct)
        return "astDeclStruct";
    else if (class == astDecl)
        return "astDecl";
    else if (class == astDeclParam)
        return "astDeclParam";
    else if (class == astType)
        return "astType";
    else if (class == astCode)
        return "astCode";
    else if (class == astBranch)
        return "astBranch";
    else if (class == astLoop)
        return "astLoop";
    else if (class == astIter)
        return "astIter";
    else if (class == astReturn)
        return "astReturn";
    else if (class == astBreak)
        return "astBreak";
    else if (class == astBOP)
        return "astBOP";
    else if (class == astUOP)
        return "astUOP";
    else if (class == astTOP)
        return "astTOP";
    else if (class == astIndex)
        return "astIndex";
    else if (class == astCall)
        return "astCall";
    else if (class == astCast)
        return "astCast";
    else if (class == astLiteral)
        return "astLiteral";

    else {
        char* Str = malloc(class+1);
        sprintf(Str, "%d", class);
        debugErrorUnhandled("astClassGetStr", "AST class", Str);
        free(Str);
        return "unhandled";
    }
}

const char* literalClassGetStr (literalClass class) {
    if (class == literalUndefined)
        return "literalUndefined";
    else if (class == literalIdent)
        return "literalIdent";
    else if (class == literalInt)
        return "literalInt";
    else if (class == literalBool)
        return "literalBool";
    else if (class == literalArray)
        return "literalArray";

    else {
        char* Str = malloc(class+1);
        sprintf(Str, "%d", class);
        debugErrorUnhandled("literalClassGetStr", "literal class", Str);
        free(Str);
        return "unhandled";
    }
}
