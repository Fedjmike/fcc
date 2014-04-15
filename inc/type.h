#pragma once

#include "../std/std.h"

using "forward.h";

typedef struct sym sym;
typedef struct type type;
typedef struct architecture architecture;

/**
 * @see type @see type::tag
 */
typedef enum typeTag {
    typeBasic,
    typePtr,
    typeArray,
    typeFunction,
    typeInvalid
} typeTag;

typedef struct typeQualifiers {
    bool isConst;
} typeQualifiers;

/**
 *
 */
typedef struct type {
    typeTag tag;

    typeQualifiers qual;

    union {
        /*typeInvalid*/
        /*typeBasic*/
        const sym* basic;
        /*typePtr
          typeArray*/
        struct {
            type* base;
            ///Size of the array
            ///-1 to indicate unspecified size in a declaration e.g. int x[] = {1, 2};
            ///NOT as a synonym for * e.g. void (*)(int[]) == void (*)(int*)
            ///-2 indicates an error finding the size
            int array;
        };
        /*typeFunction*/
        struct {
            type* returnType;
            type** paramTypes;
            int params;
            bool variadic;
        };
    };
} type;

type* typeCreateBasic (const sym* basic);
type* typeCreatePtr (type* base);
type* typeCreateArray (type* base, int size);
type* typeCreateFunction (type* returnType, type** paramTypes, int params, bool variadic);
type* typeCreateInvalid (void);

void typeDestroy (type* DT);

type* typeDeepDuplicate (const type* DT);

const sym* typeGetBasic (const type* DT);
const type* typeGetBase (const type* DT);
const type* typeGetReturn (const type* DT);
const sym* typeGetRecord (const type* DT);
const type* typeGetCallable (const type* DT);
int typeGetArraySize (const type* DT);

type* typeDeriveFrom (const type* DT);
type* typeDeriveFromTwo (const type* L, const type* R);
type* typeDeriveUnified (const type* L, const type* R);
type* typeDeriveBase (const type* DT);
type* typeDerivePtr (const type* base);
type* typeDeriveArray (const type* base, int size);
type* typeDeriveReturn (const type* fn);

/*All the following typeIsXXX respond positively when given a
  typeInvalid, so that one error doesn't cascade. If it is important
  to know the actual tag of a type (e.g. directly accessing a field)
  first check for typeInvalid then continue.*/

bool typeIsBasic (const type* DT);
bool typeIsPtr (const type* DT);
bool typeIsArray (const type* DT);
bool typeIsFunction (const type* DT);
bool typeIsInvalid (const type* DT);

bool typeIsComplete (const type* DT);
bool typeIsVoid (const type* DT);
bool typeIsStruct (const type* DT);
bool typeIsUnion (const type* DT);
bool typeIsMutable (const type* DT);

bool typeIsNumeric (const type* DT);
bool typeIsOrdinal (const type* DT);
bool typeIsEquality (const type* DT);
bool typeIsAssignment (const type* DT);
bool typeIsCondition (const type* DT);

bool typeIsCompatible (const type* DT, const type* Model);
bool typeIsEqual (const type* L, const type* R);

const char* typeTagGetStr (typeTag tag);

int typeGetSize (const architecture* arch, const type* DT);

char* typeToStr (const type* DT);
char* typeToStrEmbed (const type* DT, const char* embedded);
