#include "stdlib.h"
#include "string.h"

#include "../inc/debug.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"

static ast* parserModule ();
static ast* parserModuleLine (sym* Scope);

static ast* parserSymDef (sym* Scope);

static type parserType (sym* Scope);
static void parserStruct (sym* Scope);

static ast* parserFunction (sym* Scope, type DT, char* Ident, int Storage);
static ast* parserVariable (sym* Scope, type DT, char* Ident, int Storage);
static ast* parserArray (sym* Scope, type DT, char* Ident, int Storage);

static ast* parserCode (sym* Scope);
static ast* parserLine (sym* Scope);

static ast* parserIf (sym* Scope);
static ast* parserWhile (sym* Scope);
static ast* parserDoWhile (sym* Scope);
static ast* parserFor (sym* Scope);

static ast* parserValue (sym* Scope);
static ast* parserAssign (sym* Scope);
static ast* parserBool (sym* Scope);
static ast* parserEquality (sym* Scope);
static ast* parserRel (sym* Scope);
static ast* parserExpr (sym* Scope);
static ast* parserTerm (sym* Scope);
static ast* parserUnary (sym* Scope);
static ast* parserObject (sym* Scope);
static ast* parserFactor (sym* Scope);

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

    sym* Symbol = symCreate(symStruct);
    Symbol->ident = lexerMatchIdent();

    symAddChild(Scope, Symbol);

    /*Body?*/
    if (lexerIs("{")) {
        lexerMatch();

        /*Eat fields*/
        while (!lexerIs("}")) {
            type fieldDT = parserType(Scope);

            /*Comma separated variables*/
            do {
                sym* Field = symCreate(symVar);
                Field->dt = fieldDT;
                Field->ident = lexerMatchIdent();

                if (lexerTryMatchStr("[")) {
                    Field->dt.array = atoi(lexerBuffer);

                    if (lexerToken == tokenInt) {
                        errorExpected("number");
                        lexerNext();

                    } else if (Field->dt.array <= 0) {
                        errorExpected("positive array size");
                        lexerNext();

                    } else
                        lexerMatch();

                    lexerMatchStr("]");
                }

                symAddChild(Symbol, Field);
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
 * Function = "(" [ Type <Ident> [{ "," Type <Ident> }] ] ")" ";" | Code
 */
ast* parserFunction (sym* Scope, type DT, char* Ident, int Storage) {
    puts("Function+");

    ast* Node = astCreate(astFunction);
    Node->symbol = symCreate(symFunction);
    Node->symbol->dt = DT;
    Node->symbol->ident = Ident;
    Node->symbol->storage = Storage;

    symAddChild(Scope, Node->symbol);
    Scope = Node->symbol;

    lexerMatchStr("(");

    /*Parameter list*/
    if (!lexerIs(")")) do {
        ast* Para = astCreate(astVar);
        Para->symbol = symCreate(symPara);
        Para->symbol->dt = parserType(Scope);
        Para->symbol->ident = lexerMatchIdent();

        symAddChild(Scope, Para->symbol);
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
        Node->symbol = symCreate(symVar);
        Node->symbol->dt = DT;
        Node->symbol->ident = Ident;
        Node->symbol->storage = Storage;

        symAddChild(Scope, Node->symbol);

        if (lexerIs("=")) {
            lexerMatch();
            Node->r = parserValue(Scope);
        }
    }

    puts("-");

    return Node;
}

/**
 * Array = "[" [ <Number> ] "]" [ "=" "{" Value [{ "," Value }] "}" ]
 */
ast* parserArray (sym* Scope, type DT, char* Ident, int Storage) {
    puts("Array+");

    ast* Node = astCreate(astVar);
    Node->symbol = symCreate(symVar);
    Node->symbol->dt = DT;
    Node->symbol->dt.array = -1; /*Temporarily indicates size unspecified*/
    Node->symbol->ident = Ident;
    Node->symbol->storage = Storage;

    symAddChild(Scope, Node->symbol);

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

/**
 * Value = Assign
 */
ast* parserValue (sym* Scope) {
    puts("Value+");

    ast* Node = parserAssign(Scope);

    puts("-");

    return Node;
}

/**
 * Assign = Bool [{ "=" | "+=" | "-=" | "*=" | "/=" Bool }]
 */
ast* parserAssign (sym* Scope) {
    puts("Assign+");

    ast* Node = parserBool(Scope);

    while (lexerIs("=") ||
           lexerIs("+=") || lexerIs("-=") || lexerIs("*=") || lexerIs("/=")) {

        ast* tmp = Node;
        Node = astCreate(astBOP);
        Node->l = tmp;
        Node->o = lexerDupMatch();
        Node->r = parserBool(Scope);
    }

    puts("-");

    return Node;
}

/**
 * Bool = Equality [{ "&&" | "||" Equality }]
 */
ast* parserBool (sym* Scope) {
    puts("Bool+");

    ast* Node = parserEquality(Scope);

    while (lexerIs( "&&") || lexerIs("||")) {
        ast* tmp = Node;
        Node = astCreate(astBOP);
        Node->l = tmp;
        Node->o = lexerDupMatch();
        Node->r = parserEquality(Scope);
    }

    puts("-");

    return Node;
}

/**
 * Equality = Rel [{ "==" | "!=" Rel }]
 */
ast* parserEquality (sym* Scope) {
    puts("Equality+");

    ast* Node = parserRel(Scope);

    while (lexerIs("==") || lexerIs("!=")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserRel(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Rel = Expr [{ ">" | ">=" | "<" | "<=" Expr }]
 */
ast* parserRel (sym* Scope) {
    puts("Rel+");

    ast* Node = parserExpr(Scope);

    while (lexerIs(">") || lexerIs(">=") || lexerIs("<") || lexerIs("<=")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserExpr(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Expr = Term [{ "+" | "-" Term }]
 */
ast* parserExpr (sym* Scope) {
    puts("Expr+");

    ast* Node = parserTerm(Scope);

    while (lexerIs("+") || lexerIs("-")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserTerm(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Term = Unary [{ "*" | "/" Unary }]
 */
ast* parserTerm (sym* Scope) {
    puts("Term+");

    ast* Node = parserUnary(Scope);

    while (lexerIs("*") || lexerIs("/")) {
        char* o = lexerDupMatch();
        Node = astCreateBOP(Node, o, parserUnary(Scope));
    }

    puts("-");

    return Node;
}

/**
 * Unary = ( "!" | "-" | "*" | "&" | "++" | "--" Unary ) | Object [{ "++" | "--" }]
 */
ast* parserUnary (sym* Scope) {
    /* Interestingly, this function makes extensive use of itself */

    puts("Unary+");

    ast* Node = 0;

    if (lexerIs("!") ||
        lexerIs("-") ||
        lexerIs("*") ||
        lexerIs("&") ||
        lexerIs("++") ||
        lexerIs("--")) {
        char* o = lexerDupMatch();
        Node = astCreateUOP(o, parserUnary(Scope));

        if (!strcmp(Node->o, "*")) {
            if (typeIsPtr(Node->r->dt)) {
                Node->dt = typeDerefPtr(Node->r->dt);

            } else
                errorInvalidOpExpected("dereference", "pointer", Node->r->dt);
        }

    } else
        Node = parserObject(Scope);

    while (lexerIs("++") || lexerIs("--")) {
        char* o = lexerDupMatch();
        Node = astCreateUOP(o, Node);
    }

    puts("-");

    return Node;
}

/**
 * Object = Factor [{   ( "[" Value "]" )
                      | ( "." <Ident> )
                      | ( "->" <Ident> ) }]
 */
ast* parserObject (sym* Scope) {
    puts("Object+");

    ast* Node = parserFactor(Scope);

    while (lexerIs("[") || lexerIs(".") || lexerIs("->")) {
        /*Array or pointer indexing*/
        if (lexerIs("[")) {
            lexerMatch();

            ast* tmp = Node;
            Node = astCreate(astIndex);
            Node->l = tmp;
            Node->r = parserValue(Scope);

            if (typeIsArray(Node->l->dt))
                Node->dt = typeIndexArray(Node->l->dt);

            else if (typeIsPtr(Node->l->dt))
                Node->dt = typeDerefPtr(Node->l->dt);

            else
                errorInvalidOpExpected("indexing", "array or pointer", Node->l->dt);

            lexerMatchStr("]");

        /*struct[*] member access*/
        } else /*if (lexerIs(".") || lexerIs("->"))*/ {
            ast* tmp = Node;
            Node = astCreate(astBOP);
            Node->o = strdup(lexerBuffer);
            Node->l = tmp;

            /*Was the left hand a valid operand?*/
            if (typeIsRecord(Node->l->dt)) {
                errorInvalidOpExpected("member access", "record type", Node->l->dt);
                lexerNext();

            } else if (!strcmp("->", Node->o) && Node->l->dt.ptr != 1) {
                errorInvalidOpExpected("member dereference", "struct pointer", Node->l->dt);
                lexerNext();

            } else if (!strcmp(".", Node->o) && Node->l->dt.ptr != 0) {
                errorInvalidOp("member access", "struct pointer", Node->l->dt);
                lexerNext();

            } else
                lexerMatch();

            /*Is the right hand a valid operand?*/
            if (lexerToken == tokenIdent) {
                Node->r = astCreate(astLiteral);
                Node->r->litClass = literalIdent;
                Node->r->literal = (void*) strdup(lexerBuffer);
                Node->r->symbol = symChild(Node->l->dt.basic, (char*) Node->r->literal);

                if (Node->r->symbol == 0) {
                    errorExpected("field name");
                    lexerNext();
                    Node->r->dt.basic = symFindGlobal("int");

                } else {
                    lexerMatch();
                    Node->r->dt = Node->r->symbol->dt;
                    reportType(Node->r->dt);
                }

            } else {
                errorExpected("field name");
                lexerNext();
                Node->r->dt.basic = symFindGlobal("int");
            }

            /*Propogate type*/
            Node->dt = Node->r->dt;
        }
    }

    puts("-");

    return Node;
}

/**
 * Factor =   ( "(" Value ")" )
            | <Int>
            | ( <Ident> [ "(" [ Value [{ "," Value }] ] ")" ] )
 */
ast* parserFactor (sym* Scope) {
    puts("Factor+");

    ast* Node = 0;

    /*Parenthesized expression*/
    if (lexerIs("(")) {
        lexerMatch();
        Node = parserValue(Scope);
        lexerMatchStr(")");

    /*Integer literal*/
    } else if (lexerToken == tokenInt) {
        Node = astCreate(astLiteral);
        Node->dt.basic = symFindGlobal("int");
        Node->litClass = literalInt;
        Node->literal = malloc(sizeof(int));
        *(int*) Node->literal = lexerMatchInt();

    /*Boolean literal*/
    } else if (lexerIs("true") || lexerIs("false")) {
        Node = astCreate(astLiteral);
        Node->dt.basic = symFindGlobal("bool");
        Node->litClass = literalBool;
        Node->literal = malloc(sizeof(char));
        *(char*) Node->literal = lexerIs("true") ? 1 : 0;

    /*Identifier or function call*/
    } else if (lexerToken == tokenIdent) {
        Node = astCreate(astLiteral);
        Node->litClass = literalIdent;
        Node->literal = (void*) strdup(lexerBuffer);
        Node->symbol = symFind(Scope, (char*) Node->literal);

        /*Valid symbol?*/
        if (Node->symbol) {
            lexerMatch();
            Node->dt = Node->symbol->dt;

            reportSymbol(Node->symbol);

        } else {
            errorUndefSym();
            lexerNext();

            Node->dt.basic = symFindGlobal("int");
        }

        /*Actually it was a function call*/
        if (lexerIs("(")) {
            lexerMatch();

            ast* tmp = Node;
            Node = astCreate(astCall);
            Node->l = tmp;

            /*Eat params*/
            if (!lexerIs(")")) do {
                astAddChild(Node, parserValue(Scope));
            } while (lexerTryMatchStr(","));

            lexerMatchStr(")");
        }

    } else {
        errorExpected("expression");
        lexerNext();
    }

    puts("-");

    return Node;
}
