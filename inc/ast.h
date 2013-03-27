#pragma once

#include "../std/std.h"

#include "parser.h"

struct type;
struct sym;

typedef enum {
    astUndefined,
    astInvalid,
    astEmpty,
    astModule,
    astFnImpl,
    astDeclStruct,
    astDecl,
    astDeclParam,
    astType,
    astCode,
    astBranch,
    astLoop,
    astIter,
    astReturn,
    astBreak,
    astBOP = 20,
    astUOP,
    astTOP,
    astIndex,
    astCall,
    astCast,
    astLiteral,
} astClass;

typedef enum {
    literalUndefined,
    literalIdent,
    literalInt,
    literalBool,
    literalArray
} literalClass;

typedef struct ast {
    astClass class;

    tokenLocation location;

    /*Linked list for container nodes like a module or parameter list and children of containers*/
    struct ast* firstChild;
    struct ast* lastChild;
    struct ast* nextSibling;
    struct ast* prevSibling;
    int children;

    /*Binary tree*/
    struct ast* l;
    char* o;
    struct ast* r;      /*Always used for unary operators*/
    struct type* dt;    /*Result data type*/

    struct sym* symbol;

    /*Literals*/
    literalClass litClass;
    void* literal;
} ast;

ast* astCreate (astClass class, tokenLocation location);
ast* astCreateInvalid (tokenLocation location);
ast* astCreateEmpty (tokenLocation location);

ast* astCreateFnImpl (tokenLocation location, ast* decl, ast* impl);

ast* astCreateDeclStruct (tokenLocation location, ast* name);
ast* astCreateDecl (tokenLocation location, ast* basic);
ast* astCreateDeclParam (tokenLocation location, ast* basic, ast* declExpr);
ast* astCreateType (tokenLocation location, ast* basic, ast* expr);

ast* astCreateBOP (tokenLocation location, ast* l, char* o, ast* r);
ast* astCreateUOP (tokenLocation location, char* o, ast* r);
ast* astCreateTOP (tokenLocation location, ast* cond, ast* l, ast* r);
ast* astCreateIndex (tokenLocation location, ast* base, ast* index);
ast* astCreateCall (tokenLocation location, ast* function);
ast* astCreateCast (tokenLocation location, ast* result);
ast* astCreateLiteral (tokenLocation location, literalClass litClass);
ast* astCreateLiteralIdent (tokenLocation location, char* ident);

void astDestroy (ast* Node);

void astAddChild (ast* Parent, ast* Child);

int astIsValueClass (astClass class);

/**
 * Return the string associated with an AST class
 *
 * Does not allocate a new string, so no need to free it.
 */
const char* astClassGetStr (astClass class);

const char* literalClassGetStr (literalClass class);
