#include "type.h"

#include "stdio.h"

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
void reportSymbol (struct sym* Symbol);
void reportNode (struct ast* Node);
void reportRegs ();
