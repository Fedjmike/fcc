#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserModule (parserCtx* ctx);
static ast* parserModuleLine (parserCtx* ctx);

static void parserStruct (parserCtx* ctx);
static void parserEnum (parserCtx* ctx);
static ast* parserSymDef (parserCtx* ctx);

static type parserType (parserCtx* ctx);

static ast* parserFunction (parserCtx* ctx, type DT, char* ident, storageClass storage);
static ast* parserVariable (parserCtx* ctx, type DT, char* ident, storageClass storage);
static ast* parserArray (parserCtx* ctx, type DT, char* ident, storageClass storage);

static ast* parserCode (parserCtx* ctx);
static ast* parserLine (parserCtx* ctx);

static ast* parserIf (parserCtx* ctx);
static ast* parserWhile (parserCtx* ctx);
static ast* parserDoWhile (parserCtx* ctx);
static ast* parserFor (parserCtx* ctx);

static parserCtx* parserInit (char* File, sym* Global) {
    parserCtx* ctx = malloc(sizeof(parserCtx));

    ctx->lexer = lexerInit(File);

    ctx->location.line = ctx->lexer->stream->line;
    ctx->location.lineChar = ctx->lexer->stream->lineChar;

    ctx->scope = Global;

    ctx->insideLoop = false;

    ctx->errors = 0;
    ctx->warnings = 0;

    return ctx;
}

static void parserEnd (parserCtx* ctx) {
    lexerEnd(ctx->lexer);
    free(ctx);
}

parserResult parser (char* File, sym* Global) {
    parserCtx* ctx = parserInit(File, Global);

    ast* Module = parserModule(ctx);

    parserResult result = {Module, ctx->errors, ctx->warnings};

    parserEnd(ctx);

    return result;
}

/**
 * Module = [{ ModuleLine }]
 */
ast* parserModule (parserCtx* ctx) {
    puts("Module+");

    ast* Module = astCreate(astModule, ctx->location);
    Module->symbol = ctx->scope;

    while (ctx->lexer->token != tokenEOF)
        astAddChild(Module, parserModuleLine(ctx));

    puts("-");

    return Module;
}

/**
 * ModuleLine = Struct | SymDef
 */
ast* parserModuleLine (parserCtx* ctx) {
    puts("ModuleLine+");

    ast* Node = 0;

    if (tokenIs(ctx, "struct"))
        parserStruct(ctx);

    else
        Node = parserSymDef(ctx);

    puts("-");

    //getchar();

    return Node;
}

/**
 * Struct = "struct" <Ident> [ "{"
 *                         Type <Ident> "[" <Int> "]"
                    [{ "," Type <Ident> "[" <Int> "]" }]
                               "}" ] ";"
 */
void parserStruct (parserCtx* ctx) {
    puts("Struct+");

    tokenMatchStr(ctx, "struct");

    sym* Scope = ctx->scope;
    sym* Symbol = ctx->scope = symCreateStruct(Scope, tokenMatchIdent(ctx));

    /*Body?*/
    if (tokenIs(ctx, "{")) {
        tokenMatch(ctx);

        /*Eat fields*/
        while (!tokenIs(ctx, "}")) {
            type fieldDT = parserType(ctx);

            /*Comma separated variables*/
            do {
                sym* Field = symCreate(symVar, ctx->scope);
                Field->dt = fieldDT;
                Field->ident = tokenMatchIdent(ctx);

                if (tokenTryMatchStr(ctx, "[")) {
                    Field->dt.array = atoi(ctx->lexer->buffer);

                    if (ctx->lexer->token != tokenInt) {
                        errorExpected(ctx, "number");
                        tokenNext(ctx);

                    } else if (Field->dt.array <= 0) {
                        errorExpected(ctx, "positive array size");
                        tokenNext(ctx);

                    } else
                        tokenMatch(ctx);

                    tokenMatchStr(ctx, "]");
                }
            } while (tokenTryMatchStr(ctx, ","));

            tokenMatchStr(ctx, ";");
        }

        tokenMatchStr(ctx, "}");

    /*No? Just a prototype*/
    } else
        Symbol->proto = true;

    tokenMatchStr(ctx, ";");

    /*Revert to the old scope*/
    ctx->scope = Scope;

    puts("-");
}

/**
 * Enum = "enum" <Ident> [ "{"
 *                     <Ident> [ "=" Value ]
                [{ "," <Ident> [ "=" Value ] }]
                           "}" ] ";"
 */
void parserEnum (parserCtx* ctx) {
    puts("Enum+");

    tokenMatchStr(ctx, "enum");

    sym* Scope = ctx->scope;
    sym* Symbol = ctx->scope = symCreate(symEnum, Scope);
    Symbol->ident = tokenMatchIdent(ctx);

    if (tokenIs(ctx, "{")) {
        tokenMatch(ctx);

        type elementDT = typeCreateFromBasic(Symbol);

        do {
            sym* Element = symCreate(symVar, ctx->scope);
            Element->dt = elementDT;
            Element->ident = tokenMatchIdent(ctx);
        } while (tokenTryMatchStr(ctx, ","));

        tokenMatchStr(ctx, "}");

    } else
        Symbol->proto = true;

    tokenMatchStr(ctx, ";");

    ctx->scope = Scope;

    puts("-");
}

/**
 * SymDef = [ "static" ] Type <Ident> Function | ( Variable [{ "," <Ident> Variable }] )
 */
ast* parserSymDef (parserCtx* ctx) {
    puts("SymDef+");

    int storage = storageAuto;

    if (tokenIs(ctx, "static")) {
        storage = storageStatic;
        tokenMatch(ctx);
    }

    type DT = parserType(ctx);
    char* ident = tokenMatchIdent(ctx);
    ast* Node = 0;

    if (tokenIs(ctx, "("))
        Node = parserFunction(ctx, DT, ident, storage);

    else {
        Node = parserVariable(ctx, DT, ident, storage);

        while (tokenTryMatchStr(ctx, ",")) {
            ident = tokenMatchIdent(ctx);

            ast* tmp = Node;
            Node = parserVariable(ctx, DT, ident, storage);
            Node->l = tmp;
        }
    }

    puts("-");

    return Node;
}

/**
 * Type = <Ident> [{ "*" }]
 */
type parserType (parserCtx* ctx) {
    puts("Type+");

    type DT = typeCreateFromBasic(symFind(ctx->scope, ctx->lexer->buffer));
    DT.lvalue = true;

    if (DT.basic)
        tokenMatch(ctx);

    else {
        if (ctx->lexer->token == tokenIdent)
            errorUndefSym(ctx);

        else
            errorExpected(ctx, "type name");

        tokenNext(ctx);
    }

    while (tokenTryMatchStr(ctx, "*"))
        DT.ptr++;

    puts("-");

    return DT;
}

/**
 * Function = "(" [ Type <Ident> [{ "," Type <Ident> }] ] ")" ";" | Code
 */
ast* parserFunction (parserCtx* ctx, type DT, char* ident, storageClass storage) {
    puts("Function+");

    ast* Node = astCreate(astFunction, ctx->location);
    Node->symbol = symCreate(symFunction, ctx->scope);
    Node->symbol->dt = DT;
    Node->symbol->ident = ident;
    Node->symbol->storage = storage;

    tokenMatchStr(ctx, "(");

    /*Parameter list*/
    if (!tokenIs(ctx, ")")) do {
        ast* Para = astCreate(astVar, ctx->location);
        Para->symbol = symCreate(symParam, Node->symbol);
        Para->symbol->dt = parserType(ctx);
        Para->symbol->ident = tokenMatchIdent(ctx);

        astAddChild(Node, Para);
    } while (tokenTryMatchStr(ctx, ","));

    tokenMatchStr(ctx, ")");

    /*Push scope*/
    sym* Scope = ctx->scope;
    ctx->scope = Node->symbol;

    /*Prototype*/
    if (tokenIs(ctx, ";")) {
        Node->symbol->proto = true;
        tokenMatch(ctx);

    } else
        Node->r = parserCode(ctx);

    /*Pop scope*/
    ctx->scope = Scope;

    puts("-");

    return Node;
}

/**
 * Variable = Array | ( "=" Value )
 */
ast* parserVariable (parserCtx* ctx, type DT, char* ident, storageClass storage) {
    puts("Variable+");

    ast* Node = 0;

    if (tokenIs(ctx, "["))
        Node = parserArray(ctx, DT, ident, storage);

    else {
        Node = astCreate(astVar, ctx->location);
        Node->symbol = symCreateVar(ctx->scope, ident, DT, storage);

        if (tokenTryMatchStr(ctx, "="))
            Node->r = parserValue(ctx);
    }

    reportSymbol(Node->symbol);

    puts("-");

    return Node;
}

/**
 * Array = "[" [ <Number> ] "]" [ "=" "{" Value [{ "," Value }] "}" ]
 */
ast* parserArray (parserCtx* ctx, type DT, char* ident, storageClass storage) {
    puts("Array+");

    ast* Node = astCreate(astVar, ctx->location);
    Node->symbol = symCreateVar(ctx->scope, ident, DT, storage);
    Node->symbol->dt.array = -1; /*Temporarily indicates size unspecified*/

    tokenMatchStr(ctx, "[");

    if (ctx->lexer->token == tokenInt) {
        Node->symbol->dt.array = atoi(ctx->lexer->buffer);

        if (Node->symbol->dt.array <= 0) {
            errorExpected(ctx, "positive array size");
            tokenNext(ctx);

        } else
            tokenMatch(ctx);
    }

    tokenMatchStr(ctx, "]");

    if (tokenIs(ctx, "=")) {
        tokenMatch(ctx);
        tokenMatchStr(ctx, "{");

        Node->r = astCreateLiteral(ctx->location, literalArray);

        int n = 0;

        do {
            astAddChild(Node->r, parserValue(ctx));
            n++;
        } while (tokenTryMatchStr(ctx, ","));

        tokenMatchStr(ctx, "}");

        /*Let's check that the expected size of the array matched the size of
          the array literal. Was a size given at all?*/
        if (Node->symbol->dt.array == -1)
            Node->symbol->dt.array = n;

        else if (Node->symbol->dt.array < n) {
            char* one = malloc(20+n);
            char* two = malloc(20+n);
            sprintf(one, "%d given", Node->symbol->dt.array);
            sprintf(two, "%d element initializer", n);
            errorMismatch(ctx, "array size", one, two);
            free(one);
            free(two);
        }
    }

    puts("-");

    return Node;
}

/**
 * Code = ("{" [{ Line }] "}") | Line
 */
ast* parserCode (parserCtx* ctx) {
    puts("Code+");

    ast* Node = astCreate(astCode, ctx->location);

    if (tokenIs(ctx, "{")) {
        tokenMatch(ctx);

        while (!tokenIs(ctx, "}"))
            astAddChild(Node, parserLine(ctx));

        tokenMatch(ctx);

    } else
        astAddChild(Node, parserLine(ctx));

    puts("-");

    return Node;
}

/**
 * Line = [ If | While | For | ("return" Value) | SymDef | Value ] ";"
 */
ast* parserLine (parserCtx* ctx) {
    puts("Line+");

    ast* Node = 0;

    if (tokenIs(ctx, "if"))
        Node = parserIf(ctx);

    else if (tokenIs(ctx, "while"))
        Node = parserWhile(ctx);

    else if (tokenIs(ctx, "do"))
        Node = parserDoWhile(ctx);

    else if (tokenIs(ctx, "for"))
        Node = parserFor(ctx);

    /*Statements (that which require ';')*/
    else {
        if (tokenIs(ctx, "return")) {
            tokenMatch(ctx);

            Node = astCreate(astReturn, ctx->location);

            if (!tokenIs(ctx, ";"))
                Node->r = parserValue(ctx);

        /*Expression or symdef*/
        } else if (ctx->lexer->token == tokenIdent) {
            sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

            if (Symbol && (Symbol->class == symType ||
                           Symbol->class == symStruct))
                Node = parserSymDef(ctx);

            else
                Node = parserValue(ctx);

        /*Allow empty lines, ";"*/
        } else if (tokenIs(ctx, ";"))
            ;

        else
            Node = parserValue(ctx);

        tokenMatchStr(ctx, ";");
    }

    puts("-");

    getchar();

    return Node;
}

/**
 * If = "if" Value Code [ "else" Code ]
 */
ast* parserIf (parserCtx* ctx) {
    puts("If+");

    ast* Node = astCreate(astBranch, ctx->location);

    tokenMatchStr(ctx, "if");
    astAddChild(Node, parserValue(ctx));

    Node->l = parserCode(ctx);

    if (tokenIs(ctx, "else")) {
        tokenMatch(ctx);

        Node->r = parserCode(ctx);
    }

    puts("-");

    return Node;
}

/**
 * While = "while" Value Code
 */
ast* parserWhile (parserCtx* ctx) {
    puts("While+");

    ast* Node = astCreate(astLoop, ctx->location);

    tokenMatchStr(ctx, "while");
    Node->l = parserValue(ctx);
    Node->r = parserCode(ctx);

    puts("-");

    return Node;
}

/**
 * DoWhile = "do" Code "while" Value ";"
 */
ast* parserDoWhile (parserCtx* ctx) {
    puts("DoWhile+");

    ast* Node = astCreate(astLoop, ctx->location);

    tokenMatchStr(ctx, "do");
    Node->l = parserCode(ctx);
    tokenMatchStr(ctx, "while");
    Node->r = parserValue(ctx);
    tokenMatchStr(ctx, ";");

    puts("-");

    return Node;
}

/**
 * For := "for" [ "(" ] [ SymDef | Value ] ";" [ Value ] ";" [ Value ] [ ")" ] Code
 */
ast* parserFor (parserCtx* ctx) {
    puts("For+");

    ast* Node = astCreate(astIter, ctx->location);

    tokenMatchStr(ctx, "for");
    tokenTryMatchStr(ctx, "(");

    /*Initializer*/
    if (!tokenIs(ctx, ";")) {
        sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

        /*Expression or symdef?*/
        if ((Symbol && Symbol->class == symType) ||
            tokenIs(ctx, "struct"))
            astAddChild(Node, parserSymDef(ctx));

        else
            astAddChild(Node, parserValue(ctx));

    } else
        astAddChild(Node, astCreate(astEmpty, ctx->location));

    tokenMatchStr(ctx, ";");

    /*Condition*/
    if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreate(astEmpty, ctx->location));

    tokenMatchStr(ctx, ";");

    /*Iterator*/
    if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreate(astEmpty, ctx->location));

    tokenTryMatchStr(ctx, ")");

    Node->l = parserCode(ctx);

    puts("-");

    return Node;
}
