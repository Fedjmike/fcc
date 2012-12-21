#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/parser.h"
#include "../inc/analyzer.h"
#include "../inc/emitter.h"

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

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

    FILE* File = fopen(Output, "w");

    if (File == 0) {
        printf("Error opening file, '%s'.\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    /*Initialize symbol "table",
      make new built in data types and add them to the global namespace*/
    sym* Global = symInit();
    sym* Types[5];
    Types[builtinVoid] = symCreateType(Global, "void", 0, 0);
    Types[builtinBool] = symCreateType(Global, "bool", 1, typeEquality || typeAssignment);
    Types[builtinChar] = symCreateType(Global, "char", 1, typeIntegral);
    Types[builtinInt] = symCreateType(Global, "int", 8, typeIntegral);

    int errors = 0;
    int warnings = 0;
    /*Has compilation failed? We could be in a mode where warnings are
      treated as errors, so this isn't completely obvious from errors
      and warnings.
      Emission only happens if there is no failure, semantic analysis
      if there is no *error*. Important difference.*/
    bool fail = false;
    ast* Tree = 0;

    /*Parse the module*/

    if (!fail) {
        parserResult res = parser(Input, Global);
        errors += res.errors;
        warnings += res.warnings;
        Tree = res.tree;
    }

    fail = errors != 0;

    /*Semantic analysis*/

    if (errors == 0) {
        analyzerResult res = analyzer(Tree, Global, Types);
        errors += res.errors;
        warnings += res.warnings;
    }

    fail = errors != 0;

    /*Emit the assembly*/

    if (!fail)
        emitter(Tree, File);

    /*Clean up*/

    astDestroy(Tree);
    symEnd(Global);

    fclose(File);

    free(Input);
    free(Output);

    if (fail)
        puts("Compilation unsuccessful.");

    else
        puts("Compilation successful.");

    return fail;
}
