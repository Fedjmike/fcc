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
    astUsing,
    astFnImpl,
    astType, astDecl, astParam, astStruct, astUnion, astEnum,
    astCode, astBranch, astLoop, astIter, astReturn, astBreak, astContinue,
    astBOP, astUOP, astTOP, astIndex, astCall, astCast, astSizeof, astLiteral,
    astEllipsis
} astTag;

typedef enum {
    literalUndefined,
    literalIdent,
    literalInt,
    literalChar,
    literalBool,
    literalStr,
    literalCompound,
    literalInit
} literalTag;

typedef struct ast {
    astTag tag;

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
ast* astCreateParam (tokenLocation location, ast* basic, ast* declExpr);
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
