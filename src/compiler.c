#include "../inc/compiler.h"

#include "../std/std.h"

#include "../inc/hashmap.h"
#include "../inc/debug.h"
#include "../inc/ast.h"
#include "../inc/sym.h"
#include "../inc/operand.h"
#include "../inc/architecture.h"

#include "../inc/parser.h"
#include "../inc/analyzer.h"
#include "../inc/emitter.h"

#include "stdlib.h"


static void compilerInitSymbols (compilerCtx* ctx);

static void compilerInitSymbols (compilerCtx* ctx) {
    /*Initialize symbol "table",
      make new built in data types and add them to the global namespace*/

    ctx->global = symInit();
    ctx->types = calloc((int) builtinTotal, sizeof(sym*));
    ctx->types[builtinVoid] = symCreateType(ctx->global, "void", 0, typeNone);
    ctx->types[builtinBool] = symCreateType(ctx->global, "bool", 4, typeBool);
    ctx->types[builtinChar] = symCreateType(ctx->global, "char", 1, typeIntegral);
    ctx->types[builtinInt] = symCreateType(ctx->global, "int", 4, typeIntegral);
    ctx->types[builtinSizeT] = symCreateType(ctx->global, "size_t", ctx->arch->wordsize, typeIntegral);
    ctx->types[builtinVAList] = symCreateType(ctx->global, "va_list", ctx->arch->wordsize, typeAssignment);

    symCreateType(ctx->global, "int8_t", 1, typeIntegral);
    symCreateType(ctx->global, "int16_t", 2, typeIntegral);
    symCreateType(ctx->global, "int32_t", 4, typeIntegral);
    symCreateType(ctx->global, "intptr_t", ctx->arch->wordsize, typeIntegral);

    if (ctx->arch->wordsize >= 8)
        symCreateType(ctx->global, "int64_t", 8, typeIntegral);
}

void compilerInit (compilerCtx* ctx, const architecture* arch, const vector/*<char*>*/* searchPaths) {
    hashmapInit(&ctx->modules, 1009);

    ctx->arch = arch;
    ctx->searchPaths = searchPaths;

    ctx->errors = 0;
    ctx->warnings = 0;

    compilerInitSymbols(ctx);
}

void compilerEnd (compilerCtx* ctx) {
    hashmapFreeObjs(&ctx->modules, (hashmapKeyDtor) free, (hashmapValueDtor) parserResultDestroy);

    symEnd(ctx->global);
    ctx->global = 0;

    free(ctx->types);
    ctx->types = 0;
}

void compiler (compilerCtx* ctx, const char* input, const char* output) {
    /*Parse the module*/

    ast* tree = 0; {
        parserResult res = parser(input, "", ctx);
        ctx->errors += res.errors;
        ctx->warnings += res.warnings;
        tree = res.tree;

        if (res.notfound)
            printf("fcc: Input file '%s' doesn't exist\n", input);
    }

    /*Semantic analysis*/

    {
        analyzerResult res = analyzer(tree, ctx->types, ctx->arch);
        ctx->errors += res.errors;
        ctx->warnings += res.warnings;
    }

    /*Emit the assembly*/

    if (ctx->errors == 0 && internalErrors == 0)
        emitter(tree, output, ctx->arch);
}
