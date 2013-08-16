#include "../inc/lexer.h"

#include "../std/std.h"
#include "../inc/debug.h"

#include "../inc/stream.h"

#include "stdlib.h"
#include "string.h"
#include "ctype.h"

static void lexerSkipInsignificants (lexerCtx* ctx);
static void lexerEat (lexerCtx* ctx, char c);
static void lexerEatNext (lexerCtx* ctx);

static keywordTag lookKeyword (const char* str);

lexerCtx* lexerInit (const char* File) {
    lexerCtx* ctx = malloc(sizeof(lexerCtx));
    ctx->stream = streamInit(File);
    ctx->token = tokenUndefined;
    ctx->bufferSize = 64;
    ctx->buffer = malloc(sizeof(char)*ctx->bufferSize);
    lexerNext(ctx);
    return ctx;
}

void lexerEnd (lexerCtx* ctx) {
    streamEnd(ctx->stream);
    free(ctx->buffer);
    free(ctx);
}

/*Eat as many insignificants (comments, whitespace etc) as possible*/
static void lexerSkipInsignificants (lexerCtx* ctx) {
    while (ctx->stream->current != 0) {
        /*Whitespace*/
        if (   ctx->stream->current == ' '
            || ctx->stream->current == '\t'
            || ctx->stream->current == '\n'
            || ctx->stream->current == '\r')
            streamNext(ctx->stream);

        /*C preprocessor is treated as a comment*/
        else if (ctx->stream->current == '#') {
            /*Eat until a new line*/
            while (   ctx->stream->current != '\n'
                   && ctx->stream->current != 0)
                streamNext(ctx->stream);

            streamNext(ctx->stream);

        /*Comment?*/
        } else if (ctx->stream->current == '/') {
            streamNext(ctx->stream);

            /*C comment*/
            if (ctx->stream->current == '*') {
                streamNext(ctx->stream);

                do {
                    while (   ctx->stream->current != '*'
                           && ctx->stream->current != 0)
                        streamNext(ctx->stream);

                    if (ctx->stream->current == 0)
                        break;

                    streamNext(ctx->stream);
                } while (   ctx->stream->current != '/'
                         && ctx->stream->current != 0);

                streamNext(ctx->stream);

            /*C++ Comment*/
            } else if (ctx->stream->current == '/') {
                streamNext(ctx->stream);

                while (   ctx->stream->current != '\n'
                       && ctx->stream->current != '\r')
                    streamNext(ctx->stream);

            /*Fuck, we just ate an important character. Backtrack!*/
            } else {
                streamPrev(ctx->stream);
                break;
            }

        /*Not insignificant, leave*/
        } else
            break;

    }
}

static void lexerEat (lexerCtx* ctx, char c) {
    /*Buffer full? Double the size*/
    if (ctx->length+1 == ctx->bufferSize)
        ctx->buffer = realloc(ctx->buffer, ctx->bufferSize *= 2);

    ctx->buffer[ctx->length++] = c;
}

static void lexerEatNext (lexerCtx* ctx) {
    lexerEat(ctx, streamNext(ctx->stream));
}

tokenLocation lexerNext (lexerCtx* ctx) {
    if (ctx->token == tokenEOF) {
        tokenLocation loc = {ctx->stream->line, ctx->stream->lineChar};
        return loc;
    }

    lexerSkipInsignificants(ctx);

    tokenLocation loc = {ctx->stream->line, ctx->stream->lineChar};

    ctx->length = 0;

    /*End of stream. Do not increment pos, as we could segfault.
      Continued calls to this function will prepare more EOF tokens.*/
    if (ctx->stream->current == 0)
        ctx->token = tokenEOF;

    /*Ident or keyword*/
    else if (   isalpha(ctx->stream->current)
             || ctx->stream->current == '_') {
        lexerEatNext(ctx);

        while (   isalnum(ctx->stream->current)
               || ctx->stream->current == '_')
            lexerEatNext(ctx);

        lexerEat(ctx, 0);

        ctx->keyword = lookKeyword(ctx->buffer);
        ctx->token =   ctx->keyword != keywordUndefined
                     ? tokenKeyword
                     : tokenIdent;


    /*Number*/
    } else if (isdigit(ctx->stream->current)) {
        ctx->token = tokenInt;

        while (isdigit(ctx->stream->current))
            lexerEatNext(ctx);

    /*String*/
    } else if (ctx->stream->current == '"') {
        ctx->token = tokenStr;
        lexerEatNext(ctx);

        while (ctx->stream->current != '"') {
            if (ctx->stream->current == '\\')
                lexerEatNext(ctx);

            lexerEatNext(ctx);
        }

        lexerEatNext(ctx);

    /*Character*/
    } else if (ctx->stream->current == '\'') {
        ctx->token = tokenChar;
        lexerEatNext(ctx);

        while (ctx->stream->current != '\'') {
            if (ctx->stream->current == '\\')
                lexerEatNext(ctx);

            lexerEatNext(ctx);
        }

        lexerEatNext(ctx);

    /*Other symbol. Operators come into this category*/
    } else {
        ctx->token = tokenOther;
        lexerEatNext(ctx);

        /*If it is a double char operator like != or ->, combine them*/
        /* == !=  += -= *= /= %=  &= |= ^= */
        if (   (   (   ctx->buffer[0] == '=' || ctx->buffer[0] == '!'
                    || ctx->buffer[0] == '+' || ctx->buffer[0] == '-'
                    || ctx->buffer[0] == '*' || ctx->buffer[0] == '/'
                    || ctx->buffer[0] == '%'
                    || ctx->buffer[0] == '&' || ctx->buffer[0] == '|'
                    || ctx->buffer[0] == '^')
                && ctx->stream->current == '=')
        /* -> */
            || (ctx->buffer[0] == '-'
                && (ctx->stream->current == '>' || ctx->stream->current == '-'))
        /* && || ++ --*/
            || (   (   ctx->buffer[0] == '&' || ctx->buffer[0] == '|'
                    || ctx->buffer[0] == '+' || ctx->buffer[0] == '-')
                && ctx->stream->current == ctx->buffer[0])
        /* >= <= */
            || ((ctx->buffer[0] == '>' || ctx->buffer[0] == '<')
                && ctx->stream->current == '='))
            lexerEatNext(ctx);

        /*Possible triple char operator*/
        /* >> << */
        else if (   (ctx->buffer[0] == '>' || ctx->buffer[0] == '<')
            && (ctx->stream->current == ctx->buffer[0])) {
            lexerEatNext(ctx);

            /* >>= <<=*/
            if (ctx->stream->current == '=')
                lexerEatNext(ctx);

        /* ... */
        } else if (ctx->buffer[0] == '.' && ctx->stream->current == '.') {
            lexerEatNext(ctx);

            if (ctx->stream->current == '.')
                lexerEatNext(ctx);

            //Oops, it's just two dots, backtrack
            else {
                streamPrev(ctx->stream);
                ctx->length--;
            }
        }
    }

    lexerEat(ctx, 0);

    //printf("token(%d:%d): '%s'.\n", ctx->stream->line, ctx->stream->lineChar, ctx->buffer);

    return loc;
}

static keywordTag lookKeyword (const char* str) {
    /*Manual trie
      Yeah it's ugly, but it's fast. And lexing is the slowest part of compilation.*/

    const char* rest[] = {str+1, str+2, str+3, str+4};

    switch (str[0]) {
    case 'a': return !strcmp(rest[0], "uto") ? keywordAuto : keywordUndefined;
    case 'd': return !strcmp(rest[0], "o") ? keywordDo : keywordUndefined;
    case 'r': return !strcmp(rest[0], "eturn") ? keywordReturn : keywordUndefined;
    case 'u': return !strcmp(rest[0], "nion") ? keywordUnion : keywordUndefined;
    case 'v': return !strcmp(rest[0], "oid") ? keywordVoid : keywordUndefined;
    case 'w': return !strcmp(rest[0], "hile") ? keywordWhile : keywordUndefined;

    case 'b':
        switch (str[1]) {
        case 'o': return !strcmp(rest[1], "ol") ? keywordBool : keywordUndefined;
        case 'r': return !strcmp(rest[1], "eak") ? keywordBreak : keywordUndefined;
        default: return keywordUndefined;
        }

    case 'c':
        switch (str[1]) {
        case 'h': return !strcmp(rest[1], "ar") ? keywordChar : keywordUndefined;
        case 'o':
            if (str[2] == 'n') {
                switch (str[3]) {
                case 's': return !strcmp(rest[3], "t") ? keywordConst : keywordUndefined;
                case 't': return !strcmp(rest[3], "inue") ? keywordContinue : keywordUndefined;
                default: return keywordUndefined;
                }

            } else
                return keywordUndefined;

        default: return keywordUndefined;
        }

    case 'e':
        switch (str[1]) {
        case 'l': return !strcmp(rest[1], "se") ? keywordElse : keywordUndefined;
        case 'n': return !strcmp(rest[1], "um") ? keywordEnum : keywordUndefined;
        case 'x': return !strcmp(rest[1], "tern") ? keywordExtern : keywordUndefined;
        default: return keywordUndefined;
        }

    case 'f':
        switch (str[1]) {
        case 'a': return !strcmp(rest[1], "lse") ? keywordFalse : keywordUndefined;
        case 'o': return !strcmp(rest[1], "r") ? keywordFor : keywordUndefined;
        default: return keywordUndefined;
        }

    case 'i':
        switch (str[1]) {
        case 'f': return str[2] == 0 ? keywordIf : keywordUndefined;
        case 'n': return !strcmp(rest[1], "t") ? keywordInt : keywordUndefined;
        default: return keywordUndefined;
        }

    case 's':
        switch (str[1]) {
        case 'i': return !strcmp(rest[1], "zeof") ? keywordSizeof : keywordUndefined;
        case 't':
            switch (str[2]) {
            case 'a': return !strcmp(rest[2], "tic") ? keywordStatic : keywordUndefined;
            case 'r': return !strcmp(rest[2], "uct") ? keywordStruct : keywordUndefined;
            default: return keywordUndefined;
            }

        default: return keywordUndefined;
        }

    case 't':
        switch (str[1]) {
        case 'r': return !strcmp(rest[1], "ue") ? keywordTrue : keywordUndefined;
        case 'y': return !strcmp(rest[1], "pedef") ? keywordTypedef : keywordUndefined;
        default: return keywordUndefined;
        }

    default: return keywordUndefined;
    }
}

const char* keywordTagGetStr (keywordTag tag) {
    if (tag == keywordUndefined) return "<undefined>";
    else if (tag == keywordIf) return "if";
    else if (tag == keywordElse) return "else";
    else if (tag == keywordWhile) return "while";
    else if (tag == keywordDo) return "do";
    else if (tag == keywordFor) return "for";
    else if (tag == keywordReturn) return "return";
    else if (tag == keywordBreak) return "break";
    else if (tag == keywordContinue) return "continue";
    else if (tag == keywordSizeof) return "sizeof";
    else if (tag == keywordConst) return "const";
    else if (tag == keywordAuto) return "auto";
    else if (tag == keywordStatic) return "static";
    else if (tag == keywordExtern) return "extern";
    else if (tag == keywordTypedef) return "typedef";
    else if (tag == keywordStruct) return "struct";
    else if (tag == keywordUnion) return "union";
    else if (tag == keywordEnum) return "enum";
    else if (tag == keywordVoid) return "void";
    else if (tag == keywordBool) return "bool";
    else if (tag == keywordChar) return "char";
    else if (tag == keywordInt) return "int";
    else if (tag == keywordTrue) return "true";
    else if (tag == keywordFalse) return "false";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("keywordTagGetStr", "keyword tag", str);
        free(str);
        return "<unhandled>";
    }
}
