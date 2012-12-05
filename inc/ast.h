#pragma once

#include "type.h"

struct sym;

typedef enum {
    astUndefined,
    astEmpty,
    astModule,
    astType,
    astTypeDef,
    astFunction,
    astVar = 10,
    astCode,
    astBranch,
    astLoop,
    astIter,
    astReturn,
    astBOP = 20,
    astUOP,
    astIndex,
    astCall,
    astLiteral,
    astArrayLit
} astClass;

typedef struct ast {
	astClass class;

	/*Linked list for container nodes like a module or parameter list and children of containers*/
	struct ast* firstChild;
	struct ast* lastChild;
	struct ast* nextSibling;
	struct ast* prevSibling;

	/*Binary tree for expressions*/
	struct ast* l;
	char* o;
	struct ast* r;	/*Always used for unary operators*/
	type dt;        /*Result data type*/

	struct sym* symbol;

	/*Literals (idents included) only*/
	int litClass;
	void* literal;
} ast;

ast* astCreate (astClass class);
ast* astCreateBOP (ast* l, char* o, ast* r);
ast* astCreateUOP (char* o, ast* r);
void astDestroy (ast* Node);
void astAddChild (ast* Parent, ast* Child);

int astIsValueClass (astClass class);

/*Literal types*/
extern int literalUndefined;
extern int literalIdent;
extern int literalInt;
