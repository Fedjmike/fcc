#include "stdlib.h"
#include "ctype.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "../std/std.h"

#include "../inc/type.h"
#include "../inc/stream.h"
#include "../inc/lexer.h"

int tokenMaxSize = 50;		/*In bytes*/

tokenClass lexerToken;
char* lexerBuffer;

void lexerInit (char* File) {
    lexerBuffer = malloc(tokenMaxSize);
    lexerPush(File);
}

void lexerEnd () {
    free(lexerBuffer);

    streamEnd();
}

void lexerPush (char* File) {
    streamPush(File);
    lexerNext();
}

void lexerPop () {
    streamPop();
    lexerNext();
}

/*Eat as many insignificants (comments, whitespace etc) as possible*/
static void lexerSkipInsignificants () {
    while (streamChar != 0) {
        /*Whitespace*/
        if (streamChar == ' '  || streamChar == '\t' ||
                streamChar == '\n' || streamChar == '\r')
            streamNext();

        /*C preprocessor is treated as a comment*/
        else if (streamChar == '#') {
            /*Eat until a new line*/
            while (streamChar != '\n' && streamChar != 0)
                streamNext();

            streamNext();

        /*Comment?*/
        } else if (streamChar == '/') {
            streamNext();

            /*C comment*/
            if (streamChar == '*') {
                streamNext();

                do {
                    while (streamChar != '*' && streamChar != 0)
                        streamNext();

                    if (streamChar == 0)
                        break;

                    streamNext();
                } while (streamChar != '/' && streamChar != 0);

                streamNext();

            /*C++ Comment*/
            } else if (streamChar == '/') {
                streamNext();

                while (streamChar != '\n' && streamChar != '\r')
                    streamNext();

            /*Fuck, we just ate an important character. Backtrack!*/
            } else {
                streamPrev();
                break;
            }

        /*Not insignificant, leave*/
        } else
            break;

    }
}

void lexerNext () {
    if (lexerToken == tokenEOF)
        return;

    lexerSkipInsignificants();

    int length = 0;

    /*End of stream. Do not increment pos, as we could segfault.
      Continued calls to this function will prepare more EOF tokens.*/
    if (streamChar == 0)
        lexerToken = tokenEOF;

    /* Idents, a letter followed by any number (zero?) alphanumerics*/
    else if (isalpha(streamChar) || streamChar == '_') {
        lexerToken = tokenIdent;
        lexerBuffer[length++] = streamNext();

        while (isalnum(streamChar) || streamChar == '_')
            lexerBuffer[length++] = streamNext();

    }
    /*Number*/
    else if (isdigit(streamChar)) {
        lexerToken = tokenInt;

        while (isdigit(streamChar))
            lexerBuffer[length++] = streamNext();

        if (tolower(streamChar) == 'l')
            streamNext();

    /*Other symbol. Operators come into this category*/
    } else {
        lexerToken = tokenOther;
        lexerBuffer[length++] = streamNext();

        /*If it is a double char operator like != or ->, combine them*/

        /* != == */
        int cond = (lexerBuffer[0] == '=' || lexerBuffer[0] == '!' ||
                    lexerBuffer[0] == '+' || lexerBuffer[0] == '-' ||
                    lexerBuffer[0] == '*' || lexerBuffer[0] == '/') &&
                   streamChar == '=';

        /* -> -- */
        cond = cond || (lexerBuffer[0] == '-' &&
                        (streamChar == '>' || streamChar == '-'));

        /* ++ */
        cond = cond || (lexerBuffer[0] == '+' &&
                        streamChar == '+');
        /* >= <= */
        cond = cond || ((lexerBuffer[0] == '>' || lexerBuffer[0] == '<') &&
                         streamChar == '=');

        if (cond)
            lexerBuffer[length++] = streamNext();
    }

    lexerBuffer[length++] = 0;

    //printf("token(%d:%d): '%s'.\n", streamGetLine(), streamGetLineChar(), lexerBuffer);
}

/* ::::PARSER MESSAGES:::: */

static void error (char* format, ...) {
    printf("error(%d:%d): ", streamGetLine(), streamGetLineChar());

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    puts(".");

    getchar();
}

void errorExpected (char* Expected) {
    error("expected %s, found '%s'", Expected, lexerBuffer);
}

void errorMismatch (char* Type, char* One, char* Two) {
    error("%s mismatch between %s and %s at '%s'", Type, One, Two, lexerBuffer);
}

void errorUndefSym () {
    error("undefined symbol '%s'", lexerBuffer);
}

void errorInvalidOp (char* Op, char* TypeDesc, type DT) {
    char* TypeStr = typeToStr(DT);
    error("invalid %s on %s: %s", Op, TypeDesc, TypeStr);
    free(TypeStr);
}

/* ::::PARSER INTERFACES:::: */

bool lexerIs (char* Match) {
    return !strcmp(lexerBuffer, Match);
}

void lexerMatch () {
    printf("matched(%d:%d): '%s'.\n",
           streamGetLine(),
           streamGetLineChar(),
           lexerBuffer);
    lexerNext();
}

char* lexerDupMatch () {
    char* Old = strdup(lexerBuffer);

    lexerMatch();

    return Old;
}

/*Convert a token type to string*/
static char* tokenToStr (int Token) {
    if (Token == tokenOther)
        return "other";

    else if (Token == tokenEOF)
        return "end of file";

    else if (Token == tokenIdent)
        return "identifier";

    return "undefined";
}

void lexerMatchToken (tokenClass Match) {
    if (lexerToken == Match)
        lexerMatch();

    else {
        errorExpected(tokenToStr(Match));
        lexerNext();
    }
}

void lexerMatchStr (char* Match) {
    if (lexerIs(Match))
        lexerMatch();

    else {
        char* expectedInQuotes = malloc(strlen(Match)+2);
        sprintf(expectedInQuotes, "'%s'", Match);
        errorExpected(expectedInQuotes);
        free(expectedInQuotes);

        lexerNext();
    }
}

bool lexerTryMatchStr (char* Match) {
    if (lexerIs(Match)) {
        lexerMatch();
        return true;

    } else
        return false;
}

int lexerMatchInt () {
    int ret = atoi(lexerBuffer);

    if (lexerToken == tokenInt)
        lexerMatch();

    else
        errorExpected("integer");

    return ret;
}

char* lexerMatchIdent () {
    char* Old = strdup(lexerBuffer);

    lexerMatchToken(tokenIdent);

    return Old;
}
