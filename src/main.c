#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/options.h"
#include "../inc/compiler.h"

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

static int driver (config conf);

static int driver (config conf) {
    int errors = 0, warnings = 0;

    /*Compile each of the inputs to assembly*/
    for (int i = 0; i < conf.inputs.length; i++) {
        compilerResult result = compiler(vectorGet(&conf.inputs, i),
                                         vectorGet(&conf.intermediates, i),
                                         &conf.includeSearchPaths);
        errors += result.errors;
        warnings += result.warnings;
    }

    if (errors != 0 || warnings != 0)
        printf("Compilation complete with %d error%s and %d warning%s\n",
               errors, errors == 1 ? "" : "s",
               warnings, warnings == 1 ? "" : "s");

    else if (internalErrors)
        printf("Compilation complete with %d internal error%s\n",
               internalErrors, internalErrors == 1 ? "" : "s");

    /*Assemble/link*/
    else if (conf.mode != modeNoAssemble) {
        /*Produce a string list of all the intermediates*/
        char* intermediates = 0; {
            int length = 0;

            for (int i = 0; i < conf.intermediates.length; i++)
                length += 1+strlen((char*) vectorGet(&conf.intermediates, i));

            intermediates = strcpy(malloc(length), vectorGet(&conf.intermediates, 0));
            int charno = strlen(intermediates);

            for (int i = 1; i < conf.intermediates.length; i++) {
                char* current = vectorGet(&conf.intermediates, i);
                sprintf(intermediates+charno, " %s", current);
                charno += strlen(current)+1;
            }
        }

        if (conf.mode == modeNoLink)
            vsystem("gcc -c %s", intermediates);

        else {
            int linkfail = vsystem("gcc %s -o %s", intermediates, conf.output);

            if (conf.deleteAsm && !linkfail)
                vsystem("rm %s", intermediates);
        }

        free(intermediates);
    }

    return errors != 0;
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
        puts("Copyright 2013 Sam Nipps.");
        puts("This is free software; see the source for copying conditions.  There is NO");
        puts("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

    } else if (conf.mode == modeHelp) {
        puts("Usage: fcc [--version] [--help] [-cS] [-o <file>] <files...>");
        puts("Options:");
        puts("  -c         Compile and assemble only, do not link");
        puts("  --help     Display command line information");
        puts("  -I         Add a directory to be searched for headers");
        puts("  -o <file>  Output into a specific file");
        puts("  -s         Keep temporary assembly output after compilation");
        puts("  -S         Compile only, do not assemble or link");
        puts("  --version  Display version information");

    } else
        fail = driver(conf);

    configDestroy(conf);

    return fail;
}
