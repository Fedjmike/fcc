#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/parser-value.h"

static ast* parserModule ();
static ast* parserModuleLine (sym* Scope);

static ast* parserSymDef (sym* Scope);

static type parserType (sym* Scope);
static void parserStruct (sym* Scope);
static void parserEnum (sym* Scope);

static ast* parserFunction (sym* Scope, type DT, char* Ident, int Storage);
static ast* parserVariable (sym* Scope, type DT, char* Ident, int Storage);
static ast* parserArray (sym* Scope, type DT, char* Ident, int Storage);

static ast* parserCode (sym* Scope);
static ast* parserLine (sym* Scope);

static ast* parserIf (sym* Scope);
static ast* parserWhile (sym* Scope);
static ast* parserDoWhile (sym* Scope);
static ast* parserFor (sym* Scope);

ast* parser (char* File) {
    lexerInit(File);

    ast* Module = parserModule();

    lexerEnd();

    return Module;
}

/**
 * Module = [{ ModuleLine }]
 */
ast* parserModule () {
    puts("Module+");

    ast* Module = astCreate(astModule);
    sym* Global = Module->symbol = symInit();

    /*Make new built in data types and add them to the global namespace*/
    symCreateDataType("void", 0);
    symCreateDataType("bool", 1);
    symCreateDataType("char", 1);
    symCreateDataType("int", 8);
    symCreateDataType("void ptr", 8); /*Dummy*/

    while (lexerToken != tokenEOF)
        astAddChild(Module, parserModuleLine(Global));

    puts("-");

    return Module;
}

/**
 * ModuleLine = Struct | SymDef
 */
ast* parserModuleLine (sym* Scope) {
    puts("ModuleLine+");

    ast* Node = 0;

    if (lexerIs("struct"))
        parserStruct(Scope);

    else
        Node = parserSymDef(Scope);

    puts("-");

    //getchar();

    return Node;
}

/**
 * SymDef = [ "static" ] Type <Ident> Function | ( Variable [{ "," <Ident> Variable }] )
 */
ast* parserSymDef (sym* Scope) {
    puts("SymDef+");

    int Storage = storageAuto;

    if (lexerIs("static")) {
        Storage = storageStatic;
        lexerMatch();
    }

    type DT = parserType(Scope);
    char* Ident = lexerMatchIdent();
    ast* Node = 0;

    if (lexerIs("("))
        Node = parserFunction(Scope, DT, Ident, Storage);

    else {
        Node = parserVariable(Scope, DT, Ident, Storage);

        ast** insertionPoint = &Node->r;

        while (lexerIs(",")) {
            lexerMatch();

            Ident = lexerMatchIdent();
            *insertionPoint = parserVariable(Scope, DT, Ident, Storage);
            insertionPoint = &(*insertionPoint)->l;
        }
    }

    puts("-");

    return Node;
}

/**
 * Type = <Ident> [{ "*" }]
 */
type parserType (sym* Scope) {
    puts("Type+");

    type DT = {0, 0, 0};

    DT.basic = symFind(Scope, lexerBuffer);

    if (DT.basic)
        lexerMatch();

    else {
        if (lexerToken == tokenIdent)
            errorUndefSym();

        else
            errorExpected("type name");

        lexerNext();
    }

    while (lexerTryMatchStr("*"))
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
void parserStruct (sym* Scope) {
    puts("Struct+");

    lexerMatchStr("struct");

    sym* Symbol = Scope = symCreate(symStruct, Scope);
    Symbol->ident = lexerMatchIdent();

    /*Body?*/
    if (lexerIs("{")) {
        lexerMatch();

        /*Eat fields*/
        while (!lexerIs("}")) {
            type fieldDT = parserType(Scope->parent);

            /*Comma separated variables*/
            do {
                sym* Field = symCreate(symVar, Scope);
                Field->dt = fieldDT;
                Field->ident = lexerMatchIdent();

                if (lexerTryMatchStr("[")) {
                    Field->dt.array = atoi(lexerBuffer);

                    if (lexerToken != tokenInt) {
                        errorExpected("number");
                        lexerNext();

                    } else if (Field->dt.array <= 0) {
                        errorExpected("positive array size");
                        lexerNext();

                    } else
                        lexerMatch();

                    lexerMatchStr("]");
                }
            } while (lexerTryMatchStr(","));

            lexerMatchStr(";");
        }

        lexerMatchStr("}");

    /*No? Just a prototype*/
    } else
        Symbol->proto = true;

    lexerMatchStr(";");

    puts("-");
}

/**
 * Enum = "enum" <Ident> [ "{"
 *                     <Ident> [ "=" Value ]
                [{ "," <Ident> [ "=" Value ] }]
                           "}" ] ";"
 */
void parserEnum (sym* Scope) {
    puts("Enum+");

    lexerMatchStr("enum");

    sym* Symbol = Scope = symCreate(symEnum, Scope);
    Symbol->ident = lexerMatchIdent();

    if (lexerIs("{")) {
        lexerMatch();

        type elementDT = typeCreate(Symbol, 0, 0);

        do {
            sym* Element = symCreate(symVar, Scope);
            Element->dt = elementDT;
            Element->ident = lexerMatchIdent();
        } while (lexerTryMatchStr(","));

        lexerMatch("}");

    } else
        Symbol->proto = true;

    lexerMatchStr(";");

    puts("-");
}

/**
 * Function = "(" [ Type <Ident> [{ "," Type <Ident> }] ] ")" ";" | Code
 */
ast* parserFunction (sym* Scope, type DT, char* Ident, int Storage) {
    puts("Function+");

    ast* Node = astCreate(astFunction);
    Node->symbol = Scope = symCreate(symFunction, Scope);
    Node->symbol->dt = DT;
    Node->symbol->ident = Ident;
    Node->symbol->storage = Storage;

    lexerMatchStr("(");

    /*Parameter list*/
    if (!lexerIs(")")) do {
        ast* Para = astCreate(astVar);
        Para->symbol = symCreate(symPara, Scope);
        Para->symbol->dt = parserType(Scope->parent);
        Para->symbol->ident = lexerMatchIdent();

        astAddChild(Node, Para);
    } while (lexerTryMatchStr(","));

    lexerMatchStr(")");

    /*Prototype*/
    if (lexerIs(";")) {
        Node->symbol->proto = true;
        lexerMatch();

    } else
        Node->r = parserCode(Scope);

    puts("-");

    return Node;
}

/**
 * Variable = Array | ( "=" Value )
 */
ast* parserVariable (sym* Scope, type DT, char* Ident, int Storage) {
    puts("Variable+");

    ast* Node = 0;

    if (lexerIs("["))
        Node = parserArray(Scope, DT, Ident, Storage);

    else {
        Node = astCreate(astVar);
        Node->symbol = symCreate(symVar, Scope);
        Node->symbol->dt = DT;
        Node->symbol->ident = Ident;
        Node->symbol->storage = Storage;

        if (lexerIs("=")) {
            lexerMatch();
            Node->r = parserValue(Scope);
        }
    }

    reportSymbol(Node->symbol);

    puts("-");

    return Node;
}

/**
 * Array = "[" [ <Number> ] "]" [ "=" "{" Value [{ "," Value }] "}" ]
 */
ast* parserArray (sym* Scope, type DT, char* Ident, int Storage) {
    puts("Array+");

    ast* Node = astCreate(astVar);
    Node->symbol = symCreate(symVar, Scope);
    Node->symbol->dt = DT;
    Node->symbol->dt.array = -1; /*Temporarily indicates size unspecified*/
    Node->symbol->ident = Ident;
    Node->symbol->storage = Storage;

    lexerMatchStr("[");

    if (lexerToken == tokenInt) {
        Node->symbol->dt.array = atoi(lexerBuffer);

        if (Node->symbol->dt.array <= 0) {
            errorExpected("positive array size");
            lexerNext();

        } else
            lexerMatch();
    }

    lexerMatchStr("]");

    if (lexerIs("=")) {
        lexerMatch();
        lexerMatchStr("{");

        Node->r = astCreate(astArrayLit);

        int n = 0;

        do {
            astAddChild(Node->r, parserValue(Scope));
            n++;
        } while (lexerTryMatchStr(","));

        lexerMatchStr("}");

        /*Let's check that the expected size of the array matched the size of
          the array literal. Was a size given at all?*/
        if (Node->symbol->dt.array == -1)
            Node->symbol->dt.array = n;

        else if (Node->symbol->dt.array < n) {
            char* one = malloc(20+n);
            char* two = malloc(20+n);
            sprintf(one, "%d given", Node->symbol->dt.array);
            sprintf(two, "%d element initializer", n);
            errorMismatch("array size", one, two);
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
ast* parserCode (sym* Scope) {
    puts("Code+");

    ast* Node = astCreate(astCode);

    if (lexerIs("{")) {
        lexerMatch();

        while (!lexerIs("}"))
            astAddChild(Node, parserLine(Scope));

        lexerMatch();

    } else
        astAddChild(Node, parserLine(Scope));

    puts("-");

    return Node;
}

/**
 * Line = [ If | While | For | ("return" Value) | SymDef | Value ] ";"
 */
ast* parserLine (sym* Scope) {
    puts("Line+");

    ast* Node = 0;

    if (lexerIs("if"))
        Node = parserIf(Scope);

    else if (lexerIs("while"))
        Node = parserWhile(Scope);

    else if (lexerIs("do"))
        Node = parserDoWhile(Scope);

    else if (lexerIs("for"))
        Node = parserFor(Scope);

    /*Statements (that which require ';')*/
    else {
        if (lexerIs("return")) {
            lexerMatch();

            Node = astCreate(astReturn);

            if (!lexerIs(";"))
                Node->r = parserValue(Scope);

        } else if (lexerToken == tokenIdent) {
            sym* Symbol = symFind(Scope, lexerBuffer);

            if (Symbol && (Symbol->class == symType ||
                           Symbol->class == symStruct))
                Node = parserSymDef(Scope);

            else
                Node = parserValue(Scope);

        /*Allow empty lines, ";"*/
        } else if (lexerIs(";"))
            ;

        else
            Node = parserValue(Scope);

        lexerMatchStr(";");
    }

    puts("-");

    return Node;
}

/**
 * If = "if" Value Code [ "else" Code ]
 */
ast* parserIf (sym* Scope) {
    puts("If+");

    ast* Node = astCreate(astBranch);

    lexerMatchStr("if");
    astAddChild(Node, parserValue(Scope));

    Node->l = parserCode(Scope);

    if (lexerIs("else")) {
        lexerMatch();

        Node->r = parserCode(Scope);
    }

    puts("-");

    return Node;
}

/**
 * While = "while" Value Code
 */
ast* parserWhile (sym* Scope) {
    puts("While+");

    ast* Node = astCreate(astLoop);

    lexerMatchStr("while");
    Node->l = parserValue(Scope);
    Node->r = parserCode(Scope);

    puts("-");

    return Node;
}

/**
 * DoWhile = "do" Code "while" Value ";"
 */
ast* parserDoWhile (sym* Scope) {
    puts("DoWhile+");

    ast* Node = astCreate(astLoop);

    lexerMatchStr("do");
    Node->l = parserCode(Scope);
    lexerMatchStr("while");
    Node->r = parserValue(Scope);
    lexerMatchStr(";");

    puts("-");

    return Node;
}

/**
 * For := "for" [ "(" ] [ SymDef | Value ] ";" [ Value ] ";" [ Value ] [ ")" ] Code
 */
ast* parserFor (sym* Scope) {
    puts("For+");

    ast* Node = astCreate(astIter);

    lexerMatchStr("for");
    lexerTryMatchStr("(");

    /*Initializer*/
    if (lexerToken == tokenIdent) {
        sym* Symbol = symFind(Scope, lexerBuffer);

        if ((Symbol && Symbol->class == symType) ||
            lexerIs("struct"))
            astAddChild(Node, parserSymDef(Scope));

        else
            astAddChild(Node, parserValue(Scope));

    } else if (!lexerIs(";"))
        astAddChild(Node, parserValue(Scope));

    else
        astAddChild(Node, astCreate(astEmpty));

    lexerMatchStr(";");

    /*Condition*/
    if (!lexerIs(";"))
        astAddChild(Node, parserValue(Scope));

    else
        astAddChild(Node, astCreate(astEmpty));

    lexerMatchStr(";");

    /*Iterator*/
    if (!lexerIs(";"))
        astAddChild(Node, parserValue(Scope));

    else
        astAddChild(Node, astCreate(astEmpty));

    lexerTryMatchStr(")");

    Node->l = parserCode(Scope);

    puts("-");

    return Node;
}
