#include "../inc/debug.h"
#include "../inc/ast.h"

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

    Node->dt.basic = 0;
    Node->dt.ptr = 0;

    Node->symbol = 0;

    Node->litClass = literalUndefined;
    Node->literal = 0;

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

ast* astCreateLiteral (tokenLocation location, literalClass litClass) {
    ast* Node = astCreate(astLiteral, location);
    Node->litClass = litClass;
    return Node;
}

void astDestroy (ast* Node) {
    /*Clean up the first in the list, who will clean up its siblings*/
    if (Node->firstChild)
        astDestroy(Node->firstChild);

    /*Clean up *our* next sibling (not the previous, it's cleaning us!)*/
    if (Node->nextSibling)
        astDestroy(Node->nextSibling);

    if (Node->l)
        astDestroy(Node->l);

    if (Node->r)
        astDestroy(Node->r);

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
    return class == astBOP || class == astUOP ||
           class == astTOP ||
           class == astCall || class == astIndex ||
           class == astLiteral;
}

const char* astClassGetStr (astClass class) {
    if (class == astUndefined)
        return "astUndefind";
    else if (class == astEmpty)
        return "astEmpty";
    else if (class == astModule)
        return "astModule";
    else if (class == astFunction)
        return "astFunction";
    else if (class == astVar)
        return "astVar";
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
