#pragma once

#include "../std/std.h"

struct sym;
struct architecture;

/**
 * @see type @see type::tag
 */
typedef enum {
    typeBasic,
    typePtr,
    typeArray,
    typeFunction,
    typeInvalid
} typeTag;

/**
 *
 */
typedef struct type {
    typeTag tag;

    //bool const;

    union {
        /*typeInvalid*/
        /*typeBasic*/
        struct sym* basic;
        /*typePtr
          typeArray*/
        struct {
            struct type* base;
            ///Size of the array
            ///-1 to indicate unspecified size in a declaration e.g. int x[] = {1, 2};
            ///NOT as a synonym for * e.g. void (*)(int[]) == void (*)(int*)
            int array;
        };
        /*typeFunction*/
        struct {
            struct type* returnType;
            struct type** paramTypes;
            int params;
            bool variadic;
        };
    };
} type;

type* typeCreateBasic (struct sym* basic);
type* typeCreatePtr (type* base);
type* typeCreateArray (type* base, int size);
type* typeCreateFunction (type* returnType, type** paramTypes, int params, bool variadic);
type* typeCreateInvalid ();

void typeDestroy (type* DT);

type* typeDeepDuplicate (const type* DT);

struct sym* typeGetRecordSym (const type* record);

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
  first check for typeInvalid then continue. See impl. of typeIsEqual
  and others.*/

bool typeIsBasic (const type* DT);
bool typeIsPtr (const type* DT);
bool typeIsArray (const type* DT);
bool typeIsFunction (const type* DT);
bool typeIsInvalid (const type* DT);

bool typeIsVoid (const type* DT);
bool typeIsRecord (const type* DT);
bool typeIsCallable (const type* DT);

bool typeIsNumeric (const type* DT);
bool typeIsOrdinal (const type* DT);
bool typeIsEquality (const type* DT);
bool typeIsAssignment (const type* DT);
bool typeIsCondition (const type* DT);

bool typeIsCompatible (const type* DT, const type* Model);
bool typeIsEqual (const type* L, const type* R);

const char* typeTagGetStr (typeTag tag);

int typeGetSize (const struct architecture* arch, const type* DT);
char* typeToStr (const type* DT, const char* embedded);
