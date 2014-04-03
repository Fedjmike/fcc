#include "../std/std.h"

#include "vector.h"

using "vector.h";

typedef enum configMode {
    modeDefault,
    modeNoAssemble,
    modeNoLink,
    modeVersion,
    modeHelp
} configMode;

typedef struct config {
    bool fail;
    configMode mode;
    bool deleteAsm;

    vector/*<char*>*/ inputs, intermediates;
    char* output;
    vector/*<char*>*/ includeSearchPaths;
} config;

config configCreate (void);
void configDestroy (config conf);

void optionsParse (config* conf, int argc, char** argv);
