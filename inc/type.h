#pragma once

#include "../std/std.h"

struct sym;

typedef struct {
	struct sym* basic;
	int ptr;    /*Levels of indirection. e.g. char*** would be 3.*/
	int array;  /*Array size, 0 if not array*/
} type;

type typeCreate (struct sym* basic, int ptr, int array);

int typeGetSize (type DT);
char* typeToStr (type DT);

bool typeIsPtr (type DT);
type typeDerefPtr (type DT);

bool typeIsRecord (type DT);
