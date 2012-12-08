#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-helpers.h"
#include "../inc/parser-value.h"

static ast* parserModule (parserCtx* ctx);
static ast* parserModuleLine (parserCtx* ctx);

static ast* parserSymDef (parserCtx* ctx);

static type parserType (parserCtx* ctx);
static void parserStruct (parserCtx* ctx);
static void parserEnum (parserCtx* ctx);

static ast* parserFunction (parserCtx* ctx, type DT, char* Ident, int Storage);
static ast* parserVariable (parserCtx* ctx, type DT, char* Ident, int Storage);
static ast* parserArray (parserCtx* ctx, type DT, char* Ident, int Storage);

static ast* parserCode (parserCtx* ctx);
static ast* parserLine (parserCtx* ctx);

static ast* parserIf (parserCtx* ctx);
static ast* parserWhile (parserCtx* ctx);
static ast* parserDoWhile (parserCtx* ctx);
static ast* parserFor (parserCtx* ctx);

static parserCtx* parserInit (char* File) {
    parserCtx* ctx = malloc(sizeof(parserCtx));

    ctx->lexer = lexerInit(File);

    ctx->scope = 0;

    ctx->insideLoop = false;

    ctx->errors = 0;
    ctx->warnings = 0;

    return ctx;
}

static void parserEnd (parserCtx* ctx) {
    lexerEnd(ctx->lexer);
    free(ctx);
}

ast* parser (char* File) {

    parserCtx* ctx = parserInit(File);

    ast* Module = parserModule(ctx);

    parserEnd(ctx);



    return Module;
}

/**
 * Module = [{ ModuleLine }]
 */
ast* parserModule (parserCtx* ctx) {
    puts("Module+");

    ast* Module = astCreate(astModule);
    ctx->scope = Module->symbol = symInit();

    /*Make new built in data types and add them to the global namespace*/
    symCreateDataType("void", 0);
    symCreateDataType("bool", 1);
    symCreateDataType("char", 1);
    symCreateDataType("int", 8);
    symCreateDataType("void ptr", 8); /*Dummy*/

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
 * SymDef = [ "static" ] Type <Ident> Function | ( Variable [{ "," <Ident> Variable }] )
 */
ast* parserSymDef (parserCtx* ctx) {
    puts("SymDef+");

    int Storage = storageAuto;

    if (tokenIs(ctx, "static")) {
        Storage = storageStatic;
        tokenMatch(ctx);
    }

    type DT = parserType(ctx);
    char* Ident = tokenMatchIdent(ctx);
    ast* Node = 0;

    if (tokenIs(ctx, "("))
        Node = parserFunction(ctx, DT, Ident, Storage);

    else {
        Node = parserVariable(ctx, DT, Ident, Storage);

        ast** insertionPoint = &Node->r;

        while (tokenIs(ctx, ",")) {
            tokenMatch(ctx);

            Ident = tokenMatchIdent(ctx);
            *insertionPoint = parserVariable(ctx, DT, Ident, Storage);
            insertionPoint = &(*insertionPoint)->l;
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

    type DT = {0, 0, 0};

    DT.basic = symFind(ctx->scope, ctx->lexer->buffer);

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
 * Struct = "struct" <Ident> [ "{"
 *                         Type <Ident> "[" <Int> "]"
                    [{ "," Type <Ident> "[" <Int> "]" }]
                               "}" ] ";"
 */
void parserStruct (parserCtx* ctx) {
    puts("Struct+");

    tokenMatchStr(ctx, "struct");

    sym* Symbol = ctx->scope = symCreate(symStruct, ctx->scope);
    Symbol->ident = tokenMatchIdent(ctx);

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

    sym* Symbol = ctx->scope = symCreate(symEnum, ctx->scope);
    Symbol->ident = tokenMatchIdent(ctx);

    if (tokenIs(ctx, "{")) {
        tokenMatch(ctx);

        type elementDT = typeCreate(Symbol, 0, 0);

        do {
            sym* Element = symCreate(symVar, ctx->scope);
            Element->dt = elementDT;
            Element->ident = tokenMatchIdent(ctx);
        } while (tokenTryMatchStr(ctx, ","));

        tokenMatchStr(ctx, "}");

    } else
        Symbol->proto = true;

    tokenMatchStr(ctx, ";");

    puts("-");
}

/**
 * Function = "(" [ Type <Ident> [{ "," Type <Ident> }] ] ")" ";" | Code
 */
ast* parserFunction (parserCtx* ctx, type DT, char* Ident, int Storage) {
    puts("Function+");

    ast* Node = astCreate(astFunction);
    Node->symbol = symCreate(symFunction, ctx->scope);
    Node->symbol->dt = DT;
    Node->symbol->ident = Ident;
    Node->symbol->storage = Storage;

    tokenMatchStr(ctx, "(");

    /*Parameter list*/
    if (!tokenIs(ctx, ")")) do {
        ast* Para = astCreate(astVar);
        Para->symbol = symCreate(symPara, Node->symbol);
        Para->symbol->dt = parserType(ctx);
        Para->symbol->ident = tokenMatchIdent(ctx);

        astAddChild(Node, Para);
    } while (tokenTryMatchStr(ctx, ","));

    tokenMatchStr(ctx, ")");

    ctx->scope = Node->symbol;

    /*Prototype*/
    if (tokenIs(ctx, ";")) {
        Node->symbol->proto = true;
        tokenMatch(ctx);

    } else
        Node->r = parserCode(ctx);

    puts("-");

    return Node;
}

/**
 * Variable = Array | ( "=" Value )
 */
ast* parserVariable (parserCtx* ctx, type DT, char* Ident, int Storage) {
    puts("Variable+");

    ast* Node = 0;

    if (tokenIs(ctx, "["))
        Node = parserArray(ctx, DT, Ident, Storage);

    else {
        Node = astCreate(astVar);
        Node->symbol = symCreate(symVar, ctx->scope);
        Node->symbol->dt = DT;
        Node->symbol->ident = Ident;
        Node->symbol->storage = Storage;

        if (tokenIs(ctx, "=")) {
            tokenMatch(ctx);
            Node->r = parserValue(ctx);
        }
    }

    reportSymbol(Node->symbol);

    puts("-");

    return Node;
}

/**
 * Array = "[" [ <Number> ] "]" [ "=" "{" Value [{ "," Value }] "}" ]
 */
ast* parserArray (parserCtx* ctx, type DT, char* Ident, int Storage) {
    puts("Array+");

    ast* Node = astCreate(astVar);
    Node->symbol = symCreate(symVar, ctx->scope);
    Node->symbol->dt = DT;
    Node->symbol->dt.array = -1; /*Temporarily indicates size unspecified*/
    Node->symbol->ident = Ident;
    Node->symbol->storage = Storage;

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

        Node->r = astCreate(astArrayLit);

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

    ast* Node = astCreate(astCode);

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

            Node = astCreate(astReturn);

            if (!tokenIs(ctx, ";"))
                Node->r = parserValue(ctx);

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

    return Node;
}

/**
 * If = "if" Value Code [ "else" Code ]
 */
ast* parserIf (parserCtx* ctx) {
    puts("If+");

    ast* Node = astCreate(astBranch);

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

    ast* Node = astCreate(astLoop);

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

    ast* Node = astCreate(astLoop);

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

    ast* Node = astCreate(astIter);

    tokenMatchStr(ctx, "for");
    tokenTryMatchStr(ctx, "(");

    /*Initializer*/
    if (ctx->lexer->token == tokenIdent) {
        sym* Symbol = symFind(ctx->scope, ctx->lexer->buffer);

        if ((Symbol && Symbol->class == symType) ||
            tokenIs(ctx, "struct"))
            astAddChild(Node, parserSymDef(ctx));

        else
            astAddChild(Node, parserValue(ctx));

    } else if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreate(astEmpty));

    tokenMatchStr(ctx, ";");

    /*Condition*/
    if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreate(astEmpty));

    tokenMatchStr(ctx, ";");

    /*Iterator*/
    if (!tokenIs(ctx, ";"))
        astAddChild(Node, parserValue(ctx));

    else
        astAddChild(Node, astCreate(astEmpty));

    tokenTryMatchStr(ctx, ")");

    Node->l = parserCode(ctx);

    puts("-");

    return Node;
}
