#pragma once

#include "../std/std.h"

#include "parser.h"

using "parser.h";

typedef struct type type;
typedef struct sym sym;

typedef enum astTag {
    astUndefined,
    astInvalid,
    astEmpty,
    astModule,
    astUsing,
    astFnImpl,
    astType, astDecl, astParam, astStruct, astUnion, astEnum,
    astCode, astBranch, astLoop, astIter, astReturn, astBreak, astContinue,
    astBOP, astUOP, astTOP, astIndex, astCall, astCast, astSizeof, astLiteral,
    astEllipsis
} astTag;

typedef enum literalTag {
    literalUndefined,
    literalIdent,
    literalInt,
    literalChar,
    literalBool,
    literalStr,
    literalCompound,
    literalInit
} literalTag;

typedef struct ast ast;

typedef struct ast {
    astTag tag;

    tokenLocation location;

    /*Linked list for container nodes like a module or parameter list and children of containers*/
    ast* firstChild;
    ast* lastChild;
    ast* nextSibling;
    ast* prevSibling;
    int children;

    /*Binary tree*/
    ast* l;
    char* o;
    ast* r;      /*Always used for unary operators*/
    type* dt;    /*Result data type*/

    sym* symbol;

    /*Literals*/
    literalTag litTag;
    void* literal;
} ast;

ast* astCreate (astTag tag, tokenLocation location);
void astDestroy (ast* Node);

ast* astCreateInvalid (tokenLocation location);
ast* astCreateEmpty (tokenLocation location);

ast* astCreateUsing (tokenLocation location, char* name);

ast* astCreateFnImpl (tokenLocation location, ast* decl);

ast* astCreateType (tokenLocation location, ast* basic, ast* expr);
ast* astCreateDecl (tokenLocation location, ast* basic);
ast* astCreateParam (tokenLocation location, ast* basic, ast* expr);
ast* astCreateStruct (tokenLocation location, ast* name);
ast* astCreateUnion (tokenLocation location, ast* name);
ast* astCreateEnum (tokenLocation location, ast* name);

ast* astCreateBOP (tokenLocation location, ast* l, char* o, ast* r);
ast* astCreateUOP (tokenLocation location, char* o, ast* r);
ast* astCreateTOP (tokenLocation location, ast* cond, ast* l, ast* r);
ast* astCreateIndex (tokenLocation location, ast* base, ast* index);
ast* astCreateCall (tokenLocation location, ast* function);
ast* astCreateCast (tokenLocation location, ast* result, ast* r);
ast* astCreateSizeof (tokenLocation location, ast* r);
ast* astCreateLiteral (tokenLocation location, literalTag litTag);
ast* astCreateLiteralIdent (tokenLocation location, char* ident);

ast* astCreateEllipsis (tokenLocation location);

void astAddChild (ast* Parent, ast* Child);

bool astIsValueTag (astTag tag);

/**
 * Return the string associated with an AST tag
 *
 * Does not allocate a new string, so no need to free it.
 */
const char* astTagGetStr (astTag tag);

const char* literalTagGetStr (literalTag tag);
