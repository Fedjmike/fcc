#pragma once

#include "../std/std.h"

struct sym;

typedef enum {
    typeBasic,
    typePtr,
    typeArray,
    typeFunction,
    typeInvalid
} typeClass;

typedef struct type {
    typeClass class;

    union {
        /*typeBasic*/
        struct sym* basic;
        /*typePtr
          typeArray*/
        struct {
            struct type* base;
            int array;
        };
        /*typeFunction*/
        struct {
            struct type* returnType;
            struct type** paramTypes;
            int params;
        };
    };

    bool lvalue;
    //bool const;
} type;

type* typeCreateBasic (struct sym* basic, bool lvalue);
type* typeCreatePtr (type* base, bool lvalue);
type* typeCreateArray (type* base, int size, bool lvalue);
type* typeCreateFunction (type* returnType, type** paramTypes, int params, bool lvalue);
type* typeCreateInvalid ();

void typeDestroy (type* DT);

type* typeDeepDuplicate (const type* Old);

type* typeDeriveFrom (const type* DT);
type* typeDeriveFromTwo (const type* L, const type* R);
type* typeDeriveUnified (const type* L, const type* R);
type* typeDeriveBase (const type* DT);
type* typeDerivePtr (const type* base);
type* typeDeriveArray (const type* base, int size);

bool typeIsBasic (const type* DT);
bool typeIsPtr (const type* DT);
bool typeIsArray (const type* DT);
bool typeIsFunction (const type* DT);
bool typeIsInvalid (const type* DT);

bool typeIsLValue (const type* DT);

bool typeIsVoid (const type* DT);
bool typeIsRecord (const type* DT);

bool typeIsNumeric (const type* DT);
bool typeIsOrdinal (const type* DT);
bool typeIsEquality (const type* DT);
bool typeIsAssignment (const type* DT);
bool typeIsCondition (const type* DT);

bool typeIsCompatible (const type* DT, const type* Model);
bool typeIsEqual (const type* L, const type* R);

const char* typeClassGetStr (typeClass class);

int typeGetSize (const type* DT);
char* typeToStr (const type* DT, const char* embedded);
