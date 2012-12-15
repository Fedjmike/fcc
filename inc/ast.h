#pragma once

#include "type.h"
#include "parser.h"

struct sym;

typedef enum {
    astUndefined,
    astEmpty,
    astModule,
    astFunction,
    astVar = 10,
    astCode,
    astBranch,
    astLoop,
    astIter,
    astReturn,
    astBOP = 20,
    astUOP,
    astTOP,
    astIndex,
    astCall,
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
    struct ast* r;    /*Always used for unary operators*/
    type dt;        /*Result data type*/

    struct sym* symbol;

    /*Literals (idents included) only*/
    literalClass litClass;
    void* literal;
} ast;

ast* astCreate (astClass class, tokenLocation location);
ast* astCreateBOP (tokenLocation location, ast* l, char* o, ast* r);
ast* astCreateUOP (tokenLocation location, char* o, ast* r);
ast* astCreateTOP (tokenLocation location, ast* cond, ast* l, ast* r);
ast* astCreateIndex (tokenLocation location, ast* base, ast* index);
ast* astCreateCall (tokenLocation location, ast* function);
ast* astCreateLiteral (tokenLocation location, literalClass litClass);
void astDestroy (ast* Node);
void astAddChild (ast* Parent, ast* Child);

int astIsValueClass (astClass class);

/**
 * Return the string associated with an AST class
 *
 * Does not allocate a new string, so no need to free it.
 */
const char* astClassGetStr (astClass class);
