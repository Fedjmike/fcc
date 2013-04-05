#include "../std/std.h"

#include "stdio.h"
#include "stdarg.h"

struct type;
struct ast;
struct sym;

typedef enum {
    debugFull,
    debugCompressed,
    debugMinimal,
    debugSilent
} debugMode;

void debugInit (FILE* log);

debugMode debugSetMode (debugMode mode);

void debugWait ();

void debugEnter (const char* str);
void debugLeave ();
void debugMsg (const char* format, ...);
void debugVarMsg (const char* format, va_list args);

void debugAssert (const char* functionName,
                  const char* testName,
                  bool result);

void debugErrorUnhandled (const char* functionName,
                          const char* className,
                          const char* classStr);

void report (const char* str);
void reportType (const struct type* DT);
void reportSymbol (const struct sym* Symbol);
void reportNode (const struct ast* Node);
void reportRegs ();
