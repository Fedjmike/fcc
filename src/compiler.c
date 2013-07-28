#include "../inc/compiler.h"

#include "../std/std.h"

#include "../inc/debug.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/parser.h"
#include "../inc/analyzer.h"
#include "../inc/emitter.h"

compilerResult compiler (const char* input, const char* output) {
    architecture arch = {8};

    /*Initialize symbol "table",
      make new built in data types and add them to the global namespace*/
    sym* Global = symInit();
    sym* Types[4];
    Types[builtinVoid] = symCreateType(Global, "void", 0, 0);
    Types[builtinBool] = symCreateType(Global, "bool", arch.wordsize, typeEquality | typeAssignment | typeCondition);
    Types[builtinChar] = symCreateType(Global, "char", 1, typeIntegral);
    Types[builtinInt] = symCreateType(Global, "int", arch.wordsize, typeIntegral);

    int errors = 0, warnings = 0;

    /*Parse the module*/

    debugSetMode(debugCompressed);

    ast* Tree = 0; {
        parserResult res = parser(input, Global);
        errors += res.errors;
        warnings += res.warnings;
        Tree = res.tree;
    }

    /*Semantic analysis*/

    {
        analyzerResult res = analyzer(Tree, Types);
        errors += res.errors;
        warnings += res.warnings;
    }

    /*Emit the assembly*/

    if (errors == 0) {
        debugWait();
        emitter(Tree, output, &arch);
    }

    /*Clean up*/

    astDestroy(Tree);
    symEnd(Global);

    return (compilerResult) {errors, warnings};
}
