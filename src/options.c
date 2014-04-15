#include "../inc/options.h"

#include "../inc/architecture.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

using "../inc/options.h";

using "../inc/architecture.h";

using "../std/std.h";

using "stdlib.h";
using "stdio.h";
using "string.h";

typedef enum expectTag {
    expectNothing,
    expectOutput,
    expectIncludeSearchPath,
    expectTheUnexpected
} expectTag;

typedef struct optionsState {
    expectTag expect;
} optionsState;

static void configSetMode (config* conf, configMode mode, const char* option);
static void stateSetExpect (optionsState* state, expectTag expect, const char* option);

static void optionsParseMacro (config* conf, optionsState* state, const char* option);
static void optionsParseMicro (config* conf, optionsState* state, const char* option);

/*:::: PROGRAM CONFIGURATION ::::*/

config configCreate () {
    config conf;
    conf.fail = false;
    conf.mode = modeDefault;
    conf.deleteAsm = true;

    archInit(&conf.arch);

    vectorInit(&conf.inputs, 32);
    vectorInit(&conf.intermediates, 32);

    conf.output = 0;

    vectorInit(&conf.includeSearchPaths, 8);
    vectorPush(&conf.includeSearchPaths, strdup(""));

    return conf;
}

void configDestroy (config conf) {
    archFree(&conf.arch);

    vectorFreeObjs(&conf.inputs, free);
    vectorFreeObjs(&conf.intermediates, free);
    vectorFreeObjs(&conf.includeSearchPaths, free);

    free(conf.output);
}

static void configSetMode (config* conf, configMode mode, const char* option) {
    if (conf->mode != modeDefault)
        printf("fcc: Option '%s' overriding previous\n", option);

    conf->mode = mode;
}

/*:::: OPTIONS STATE ::::*/

static void stateSetExpect (optionsState* state, expectTag expect, const char* option) {
    if (state->expect != expectNothing)
        printf("fcc: Option '%s' overriding previous\n", option);

    state->expect = expect;
}

/*:::: OPTIONS PARSER ::::*/

static void optionsParseMacro (config* conf, optionsState* state, const char* option) {
    (void) state;

    if (!strcmp(option, "--version"))
        configSetMode(conf, modeVersion, option);

    else if (!strcmp(option, "--help"))
        configSetMode(conf, modeHelp, option);

    else
        printf("fcc: Unknown option '%s'\n", option);
}

static void optionsParseMicro (config* conf, optionsState* state, const char* option) {
    for (int j = 1; j < (int) strlen(option); j++) {
        char suboption = option[j];
        char asStr[2] = {suboption, '\0'};

        if (suboption == 'c')
            configSetMode(conf, modeNoLink, asStr);

        else if (suboption == 'S')
            configSetMode(conf, modeNoAssemble, asStr);

        else if (suboption == 's')
            conf->deleteAsm = false;

        else if (suboption == 'o')
            stateSetExpect(state, expectOutput, asStr);

        else if (suboption == 'I')
            stateSetExpect(state, expectIncludeSearchPath, asStr);

        else
            printf("fcc: Unknown option '%c' in '%s'\n", suboption, option);
    }
}

void optionsParse (config* conf, int argc, char** argv) {
    optionsState state = {expectNothing};

    for (int i = 1; i < argc; i++) {
        const char* option = argv[i];

        if (strprefix(option, "-")) {
            if (state.expect != expectNothing) {
                const char* noun = state.expect == expectOutput ? "output file" : "include search path";
                printf("fcc: Expected %s for preceding option, found option '%s'\n", noun, option);
                state.expect = expectNothing;
            }

            if (strprefix(option, "--"))
                optionsParseMacro(conf, &state, option);

            else if (strprefix(option, "-"))
                optionsParseMicro(conf, &state, option);

        } else {
            if (state.expect == expectOutput) {
                if (conf->output) {
                    printf("fcc: Overriding previous output file with '%s'\n", option);
                    free(conf->output);
                }

                conf->output = strdup(option);
                state.expect = expectNothing;

            } else if (state.expect == expectIncludeSearchPath) {
                vectorPush(&conf->includeSearchPaths, strdup(option));
                state.expect = expectNothing;

            } else {
                if (fexists(option)) {
                    vectorPush(&conf->inputs, strdup(option));
                    vectorPush(&conf->intermediates, filext(option, "s"));

                } else
                    printf("fcc: Input file '%s' doesn't exist\n", option);
            }
        }
    }

    if (   conf->mode != modeVersion
        && conf->mode != modeHelp) {
        if (conf->inputs.length == 0) {
            puts("fcc: No input files given");
            conf->fail = true;

        } else {
            if (   conf->inputs.length > 1
                && conf->mode == modeNoLink
                && conf->output) {
                puts("fcc: Multiple input files and given no-link option '-c'");
                puts("     but single output file specified with '-o'");
                puts("     Ignoring option '-o'");
            }

            if (conf->output == 0)
                conf->output = filext(vectorGet(&conf->inputs, 0), "");
        }
    }
}
