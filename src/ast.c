#include "stdio.h"
#include "stdlib.h"

#include "../inc/ast.h"

int literalUndefined = 0;
int literalIdent = 1;
int literalInt = 2;

ast* astCreate (astClass class) {
    ast* Node = malloc(sizeof(ast));
    Node->class = class;

    Node->firstChild = 0;
    Node->lastChild = 0;
    Node->nextSibling = 0;
    Node->prevSibling = 0;

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

ast* astCreateBOP (ast* l, char* o, ast* r) {
    ast* Node = astCreate(astBOP);
    Node->l = l;
    Node->o = o;
    Node->r = r;
    return Node;
}

ast* astCreateUOP (char* o, ast* r) {
    ast* Node = astCreate(astUOP);
    Node->o = o;
    Node->r = r;
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

    free(Node->literal);
    free(Node);
}

void astAddChild (ast* Parent, ast* Child) {
    if (!Child || !Parent)
        printf("astAddChild(): null %s given.\n",
               !Parent ? "parent" : "child");

    if (Parent->firstChild == 0) {
        Parent->firstChild = Child;
        Parent->lastChild = Child;

    } else {
        Child->prevSibling = Parent->lastChild;
        Parent->lastChild->nextSibling = Child;
        Parent->lastChild = Child;
    }
}

int astIsValueClass (astClass class) {
    return class == astBOP || class == astUOP ||
             class == astCall || class == astIndex ||
             class == astLiteral;
}
