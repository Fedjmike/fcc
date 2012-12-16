#include "stdarg.h"
#include "stdio.h"

#include "type.h"
#include "sym.h"
#include "ast.h"

typedef enum {
    debugFull,
    debugCompressed,
    debugMinimal,
    debugSilent
} debugMode;

void debugInit (FILE* log);

debugMode debugSetMode (debugMode mode);

void debugEnter (char* str);
void debugLeave ();
void debugMsg (char* format, ...);

void debugAssert (const char* functionName,
                  const char* testName,
                  bool result);

void debugErrorUnhandled (const char* functionName,
                          const char* className,
                          const char* classStr);

void report (char* str);
void reportType (type DT);
void reportSymbol (sym* Symbol);
void reportNode (ast* Node);
void reportRegs ();
