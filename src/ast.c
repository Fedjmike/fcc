#include "../inc/ast.h"

#include "../inc/debug.h"
#include "../inc/type.h"

#include "stdio.h"
#include "stdlib.h"

ast* astCreate (astTag tag, tokenLocation location) {
    ast* Node = malloc(sizeof(ast));
    Node->tag = tag;

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

    Node->litTag = literalUndefined;
    Node->literal = 0;

    return Node;
}

void astDestroy (ast* Node) {
    debugAssert("astDestroy", "null param", Node != 0);

    for (ast *Current = Node->firstChild, *Next = Current ? Current->nextSibling : 0;
         Current;
         Current = Next, Next = Next ? Next->nextSibling : 0)
        astDestroy(Current);

    if (Node->l)
        astDestroy(Node->l);

    if (Node->r)
        astDestroy(Node->r);

    if (Node->dt)
        typeDestroy(Node->dt);

    if (Node->tag == astModule)
        free(Node->location.filename);

    free(Node->o);
    free(Node->literal);
    free(Node);
}

ast* astCreateInvalid (tokenLocation location) {
    return astCreate(astInvalid, location);
}

ast* astCreateEmpty (tokenLocation location) {
    return astCreate(astEmpty, location);
}

ast* astCreateUsing (tokenLocation location, char* name) {
    ast* Node = astCreate(astUsing, location);
    Node->litTag = literalStr;
    Node->literal = (void*) name;
    return Node;
}

ast* astCreateFnImpl (tokenLocation location, ast* decl) {
    ast* Node = astCreate(astFnImpl, location);
    Node->l = decl;
    return Node;
}

ast* astCreateType (tokenLocation location, ast* basic, ast* expr) {
    ast* Node = astCreate(astType, location);
    Node->l = basic;
    Node->r = expr;
    return Node;
}

ast* astCreateDecl (tokenLocation location, ast* basic) {
    ast* Node = astCreate(astDecl, location);
    Node->l = basic;
    return Node;
}

ast* astCreateParam (tokenLocation location, ast* basic, ast* expr) {
    ast* Node = astCreate(astParam, location);
    Node->l = basic;
    Node->r = expr;
    return Node;
}

ast* astCreateStruct (tokenLocation location, ast* name) {
    ast* Node = astCreate(astStruct, location);
    Node->l = name;
    return Node;
}

ast* astCreateUnion (tokenLocation location, ast* name) {
    ast* Node = astCreate(astUnion, location);
    Node->l = name;
    return Node;
}

ast* astCreateEnum (tokenLocation location, ast* name) {
    ast* Node = astCreate(astEnum, location);
    Node->l = name;
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
    return Node;
}

ast* astCreateCast (tokenLocation location, ast* result, ast* r) {
    ast* Node = astCreate(astCast, location);
    Node->l = result;
    Node->r = r;
    return Node;
}

ast* astCreateSizeof (tokenLocation location, ast* r) {
    ast* Node = astCreate(astSizeof, location);
    Node->r = r;
    return Node;
}

ast* astCreateLiteral (tokenLocation location, literalTag litTag) {
    ast* Node = astCreate(astLiteral, location);
    Node->litTag = litTag;
    return Node;
}

ast* astCreateLiteralIdent (tokenLocation location, char* ident) {
    ast* Node = astCreateLiteral(location, literalIdent);
    Node->literal = (void*) ident;
    return Node;
}

ast* astCreateEllipsis (tokenLocation location) {
    return astCreate(astEllipsis, location);
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

bool astIsValueTag (astTag tag) {
    return    tag == astBOP || tag == astUOP || tag == astTOP
           || tag == astCall || tag == astIndex || tag == astCast
           || tag == astSizeof || tag == astLiteral;
}

const char* astTagGetStr (astTag tag) {
    if (tag == astUndefined) return "astUndefined";
    else if (tag == astInvalid) return "astInvalid";
    else if (tag == astEmpty) return "astEmpty";
    else if (tag == astModule) return "astModule";
    else if (tag == astUsing) return "astUsing";
    else if (tag == astFnImpl) return "astFnImpl";
    else if (tag == astDecl) return "astDecl";
    else if (tag == astParam) return "astParam";
    else if (tag == astStruct) return "astStruct";
    else if (tag == astUnion) return "astUnion";
    else if (tag == astType) return "astType";
    else if (tag == astCode) return "astCode";
    else if (tag == astBranch) return "astBranch";
    else if (tag == astLoop) return "astLoop";
    else if (tag == astIter) return "astIter";
    else if (tag == astReturn) return "astReturn";
    else if (tag == astBreak) return "astBreak";
    else if (tag == astContinue) return "astContinue";
    else if (tag == astBOP) return "astBOP";
    else if (tag == astUOP) return "astUOP";
    else if (tag == astTOP) return "astTOP";
    else if (tag == astIndex) return "astIndex";
    else if (tag == astCall) return "astCall";
    else if (tag == astCast) return "astCast";
    else if (tag == astSizeof) return "astSizeof";
    else if (tag == astLiteral) return "astLiteral";
    else if (tag == astEllipsis) return "astEllipsis";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("astTagGetStr", "AST tag", str);
        free(str);
        return "<unhandled>";
    }
}

const char* literalTagGetStr (literalTag tag) {
    if (tag == literalUndefined) return "literalUndefined";
    else if (tag == literalIdent) return "literalIdent";
    else if (tag == literalInt) return "literalInt";
    else if (tag == literalChar) return "literalChar";
    else if (tag == literalStr) return "literalStr";
    else if (tag == literalBool) return "literalBool";
    else if (tag == literalCompound) return "literalCompound";
    else if (tag == literalInit) return "literalInit";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("literalTagGetStr", "literal tag", str);
        free(str);
        return "<unhandled>";
    }
}
