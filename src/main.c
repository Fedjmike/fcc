#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/architecture.h"
#include "../inc/options.h"
#include "../inc/compiler.h"
#include "../inc/reg.h"

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

static bool driver (config conf);

static bool driver (config conf) {
    bool fail = false;

    architecture arch;
    architectureInit(&arch, 4);
    vectorPushFromArray(&arch.scratchRegs, (void**) &(regIndex[3]) {regRBX, regRSI, regRDI}, 3);

    compilerCtx comp;
    compilerInit(&comp, &arch, &conf.includeSearchPaths);

    /*Compile each of the inputs to assembly*/
    for (int i = 0; i < conf.inputs.length; i++)
        compiler(&comp,
                 vectorGet(&conf.inputs, i),
                 vectorGet(&conf.intermediates, i));

    compilerEnd(&comp);

    if (comp.errors != 0 || comp.warnings != 0)
        printf("Compilation complete with %d error%s and %d warning%s\n",
               comp.errors, comp.errors == 1 ? "" : "s",
               comp.warnings, comp.warnings == 1 ? "" : "s");

    else if (internalErrors)
        printf("Compilation complete with %d internal error%s\n",
               internalErrors, internalErrors == 1 ? "" : "s");

    /*Assemble/link*/
    else if (conf.mode != modeNoAssemble) {
        /*Produce a string list of all the intermediates*/
        char* intermediates = strjoin((char**) conf.intermediates.buffer, conf.intermediates.length,
                                      " ", (stdalloc) malloc);

        if (conf.mode == modeNoLink)
            fail |= (bool) systemf("gcc -c %s", intermediates);

        else {
            fail |= (bool) systemf("gcc %s -o %s", intermediates, conf.output);

            if (conf.deleteAsm && !fail)
                systemf("rm %s", intermediates);
        }

        free(intermediates);
    }

    architectureFree(&arch);

    return fail || comp.errors != 0 || internalErrors != 0;
}

int main (int argc, char** argv) {
    debugInit(stdout);

    bool fail = false;

    config conf = configCreate();
    optionsParse(&conf, argc, argv);

    if (conf.fail)
        fail = true;

    else if (conf.mode == modeVersion) {
        puts("Fedjmike's C Compiler (fcc) v0.01b");
        puts("Copyright 2014 Sam Nipps.");
        puts("This is free software; see the source for copying conditions.  There is NO");
        puts("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

    } else if (conf.mode == modeHelp) {
        puts("Usage: fcc [--help] [--version] [-csS] [-I <dir>] [-o <file>] <files...>");
        puts("Options:");
        puts("  -I <dir>   Add a directory to be searched for headers");
        puts("  -c         Compile and assemble only, do not link");
        puts("  -S         Compile only, do not assemble or link");
        puts("  -s         Keep temporary assembly output after compilation");
        puts("  -o <file>  Output into a specific file");
        puts("  --help     Display command line information");
        puts("  --version  Display version information");

    } else
        fail = driver(conf);

    configDestroy(conf);

    return fail;
}
