#include "stdio.h"

#include "type.h"
#include "sym.h"
#include "ast.h"

void debugInit (FILE* Log);

void report (char* Str);
void reportType (type DT);
void reportSymbol (sym* Symbol);
void reportNode (ast* Node);
void reportRegs ();
