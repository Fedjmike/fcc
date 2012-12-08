#include "stdlib.h"
#include "stdarg.h"
#include "string.h"

#include "../inc/parser-helpers.h"

static void error (parserCtx* ctx, char* format, ...) {
    printf("error(%d:%d): ", ctx->lexer->stream->line, ctx->lexer->stream->lineChar);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    puts(".");

    getchar();
}

void errorExpected (parserCtx* ctx, char* Expected) {
    error(ctx, "expected %s, found '%s'", Expected, ctx->lexer->buffer);
}

void errorMismatch (parserCtx* ctx, char* Type, char* One, char* Two) {
    error(ctx, "%s mismatch between %s and %s at '%s'", Type, One, Two, ctx->lexer->buffer);
}

void errorUndefSym (parserCtx* ctx) {
    error(ctx, "undefined symbol '%s'", ctx->lexer->buffer);
}

void errorInvalidOp (parserCtx* ctx, char* Op, char* TypeDesc, type DT) {
    char* TypeStr = typeToStr(DT);
    error(ctx, "invalid %s on %s: %s", Op, TypeDesc, TypeStr);
    free(TypeStr);
}

void errorInvalidOpExpected (parserCtx* ctx, char* Op, char* TypeDesc, type DT) {
    char* TypeStr = typeToStr(DT);
    error(ctx, "invalid %s on %s, expected %s", Op, TypeStr, TypeDesc);
    free(TypeStr);
}

bool tokenIs (parserCtx* ctx, char* Match) {
    return !strcmp(ctx->lexer->buffer, Match);
}

void tokenNext (parserCtx* ctx) {
    lexerNext(ctx->lexer);
}

void tokenMatch (parserCtx* ctx) {
    printf("matched(%d:%d): '%s'.\n",
           ctx->lexer->stream->line,
           ctx->lexer->stream->lineChar,
           ctx->lexer->buffer);
    lexerNext(ctx->lexer);
}

char* tokenDupMatch (parserCtx* ctx) {
    char* Old = strdup(ctx->lexer->buffer);

    tokenMatch(ctx);

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

void tokenMatchToken (parserCtx* ctx, tokenClass Match) {
    if (ctx->lexer->token == Match)
        tokenMatch(ctx);

    else {
        errorExpected(ctx, tokenToStr(Match));
        lexerNext(ctx->lexer);
    }
}

void tokenMatchStr (parserCtx* ctx, char* Match) {
    if (tokenIs(ctx, Match))
        tokenMatch(ctx);

    else {
        char* expectedInQuotes = malloc(strlen(Match)+2);
        sprintf(expectedInQuotes, "'%s'", Match);
        errorExpected(ctx, expectedInQuotes);
        free(expectedInQuotes);

        lexerNext(ctx->lexer);
    }
}

bool tokenTryMatchStr (parserCtx* ctx, char* Match) {
    if (tokenIs(ctx, Match)) {
        tokenMatch(ctx);
        return true;

    } else
        return false;
}

int tokenMatchInt (parserCtx* ctx) {
    int ret = atoi(ctx->lexer->buffer);

    if (ctx->lexer->token == tokenInt)
        tokenMatch(ctx);

    else
        errorExpected(ctx, "integer");

    return ret;
}

char* tokenMatchIdent (parserCtx* ctx) {
    char* Old = strdup(ctx->lexer->buffer);

    tokenMatchToken(ctx, tokenIdent);

    return Old;
}
