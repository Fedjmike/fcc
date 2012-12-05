#pragma once

#include "type.h"
#include "operand.h"

typedef enum {
    symUndefined,
    symGlobal,
    symType,
    symStruct,
    symFunction,
    symPara,
    symVar
} symClass;

typedef enum {
    storageUndefined,
    storageAuto,
    storageRegister,
    storageStatic,
    storageExtern
} storageClass;

typedef struct sym {
	symClass class;
	char* ident;
	bool proto;

	storageClass storage;
	type dt;		/*Ignore for types themselves*/

	int size;		/*Types and Structs only*/

	/*Linked list of symbols in our namespace*/
	struct sym* parent;
	struct sym* firstChild;
	struct sym* lastChild;
	struct sym* nextSibling;

	operand label;
	int offset;		/*Offset, in word bytes, for stack stored vars/parameters
                      and non static fields*/
} sym;

sym* symInit ();
void symEnd ();

sym* symCreate (symClass class);
sym* symCreateDataType (char* Ident, int Size);
void symDestroy (sym* Symbol);

void symAddChild (sym* Parent, sym* Child);

sym* symChild (sym* Scope, char* Look);
sym* symFind (sym* Scope, char* Look);
sym* symFindGlobal (char* Look);
