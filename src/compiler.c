#include "../inc/compiler.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/parser.h"
#include "../inc/analyzer.h"
#include "../inc/emitter.h"

compilerResult compiler (const char* input, const char* output, vector/*<char*>*/* searchPaths) {
    architecture arch = {4};

    /*Initialize symbol "table",
      make new built in data types and add them to the global namespace*/

    sym* Global = symInit();
    sym* Types[4];
    Types[builtinVoid] = symCreateType(Global, "void", 0, 0);
    Types[builtinBool] = symCreateType(Global, "bool", 4, typeEquality | typeAssignment | typeCondition);
    Types[builtinChar] = symCreateType(Global, "char", 1, typeIntegral);
    Types[builtinInt] = symCreateType(Global, "int", 4, typeIntegral);

    symCreateType(Global, "int8_t", 1, typeIntegral);
    symCreateType(Global, "int16_t", 2, typeIntegral);
    symCreateType(Global, "int32_t", 4, typeIntegral);
    symCreateType(Global, "intptr_t", arch.wordsize, typeIntegral);

    if (arch.wordsize >= 8)
        symCreateType(Global, "uint64_t", 8, typeIntegral);

    int errors = 0, warnings = 0;

    /*Parse the module*/

    ast* Tree = 0; {
        parserResult res = parser(input, Global, searchPaths);
        errors += res.errors;
        warnings += res.warnings;
        Tree = res.tree;

        if (res.notfound)
            printf("error: File not found, '%s'\n", input);
    }

    /*Semantic analysis*/

    {
        analyzerResult res = analyzer(Tree, Types);
        errors += res.errors;
        warnings += res.warnings;
    }

    /*Emit the assembly*/

    if (errors == 0 && internalErrors == 0) {
        debugWait();
        emitter(Tree, output, &arch);
    }

    /*Clean up*/

    astDestroy(Tree);
    symEnd(Global);

    return (compilerResult) {errors, warnings};
}
