#include "../std/std.h"

#include "operand.h"

#include "stdio.h"
#include "stdarg.h"

struct type;
struct ast;
struct sym;
struct architecture;

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
void debugOut (const char* format, ...);
void debugVarOut (const char* format, va_list args);

void debugError (const char* functionName,
                 const char* format, ...);

void debugAssert (const char* functionName,
                  const char* testName,
                  bool result);

void debugErrorUnhandled (const char* functionName,
                          const char* tagName,
                          const char* tagStr);

void debugErrorUnhandledInt (const char* functionName,
                             const char* className,
                             int classInt);

void report (const char* str);
void reportType (const struct type* DT);
void reportSymbol (const struct sym* Symbol);
void reportNode (const struct ast* Node);
void reportRegs ();
void reportOperand (const struct architecture* arch, operand R);

extern int internalErrors;
