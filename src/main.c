#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/parser.h"
#include "../inc/emitter.h"

int main (int argc, char** argv) {
    debugInit(stdout);

    char* Input;
    char* Output;

    if (argc <= 1) {
        puts("No input file");
        return 1;
    }

    Input = strdup(argv[1]);

    /*No output file*/
    if (argc <= 2)
        Output = filext(Input, "asm");

    else
        Output = strdup(argv[2]);

    /*Parse the module*/

    ast* Tree = parser(Input);

    /*Emit the assembly*/

    FILE* File = fopen(Output, "w");

    if (File == 0) {
        printf("Error opening file, '%s'.\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    emitter(Tree, File);

    /*Clean up*/

    astDestroy(Tree);
    symEnd();

    fclose(File);

    free(Input);
    free(Output);

    puts("Compilation over.");
    return 0;
}
