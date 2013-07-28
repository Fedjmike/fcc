#include "../inc/options.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

typedef struct {
    bool expectOutput;
} optionsState;

static void configSetMode (config* conf, configMode mode, const char* option);

static void optionsParseMacro (config* conf, optionsState* state, const char* option);
static void optionsParseMicro (config* conf, optionsState* state, const char* option);

/*:::: PROGRAM CONFIGURATION ::::*/

config configCreate () {
    config conf;
    conf.fail = false;
    conf.mode = modeDefault;
    conf.inputs = vectorCreate(32);
    conf.intermediates = vectorCreate(32);
    conf.output = 0;
    return conf;
}

void configDestroy (config conf) {
    vectorDestroyObjs(&conf.inputs, free);
    vectorDestroyObjs(&conf.intermediates, free);
    free(conf.output);
}

static void configSetMode (config* conf, configMode mode, const char* option) {
    if (conf->mode != modeDefault)
        printf("fcc: Option '%s' overriding previous\n", option);

    conf->mode = mode;
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
        char asStr[2] = {suboption, 0};

        if (suboption == 'c')
            configSetMode(conf, modeNoLink, asStr);

        else if (suboption == 'S')
            configSetMode(conf, modeNoAssemble, asStr);

        else if (suboption == 'o')
            state->expectOutput = true;

        else
            printf("fcc: Unknown option '%c' in '%s'\n", suboption, option);
    }
}

void optionsParse (config* conf, int argc, char** argv) {
    optionsState state;
    state.expectOutput = false;

    for (int i = 1; i < argc; i++) {
        const char* option = argv[i];

        if (strprefix(option, "-")) {
            if (state.expectOutput)
                printf("fcc: Expected output file, found option '%s'\n", option);

            if (strprefix(option, "--"))
                optionsParseMacro(conf, &state, option);

            else if (strprefix(option, "-"))
                optionsParseMicro(conf, &state, option);

        } else {
            if (state.expectOutput) {
                if (conf->output) {
                    printf("fcc: Overriding previous output file with '%s'\n", option);
                    free(conf->output);
                }

                conf->output = strdup(option);

            } else {
                if (fexists(option)) {
                    vectorAdd(&conf->inputs, strdup(option));
                    vectorAdd(&conf->intermediates, filext(option, "s"));

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
