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
static bool lexerTryEatNext (lexerCtx* ctx, char c);

static keywordTag lookKeyword (const char* str);

lexerCtx* lexerInit (FILE* file) {
    lexerCtx* ctx = malloc(sizeof(lexerCtx));
    ctx->stream = streamInit(file);
    ctx->line = 1;
    ctx->lineChar = 1;

    ctx->token = tokenUndefined;
    ctx->keyword = keywordUndefined;
    ctx->punct = punctUndefined;

    ctx->bufferSize = 64;
    ctx->buffer = malloc(sizeof(char)*ctx->bufferSize);
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
                       && ctx->stream->current != '\r'
                       && ctx->stream->current != 0)
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

static bool lexerTryEatNext (lexerCtx* ctx, char c) {
    if (ctx->stream->current == c) {
        lexerEatNext(ctx);
        return true;

    } else
        return false;
}

void lexerNext (lexerCtx* ctx) {
    if (ctx->token == tokenEOF)
        return;

    lexerSkipInsignificants(ctx);

    ctx->line = ctx->stream->line;
    ctx->lineChar = ctx->stream->lineChar;

    ctx->length = 0;
    ctx->keyword = keywordUndefined;
    ctx->punct = punctUndefined;

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

        while (   ctx->stream->current != '"'
               && ctx->stream->current != 0) {
            if (ctx->stream->current == '\\')
                lexerEatNext(ctx);

            lexerEatNext(ctx);
        }

        lexerEatNext(ctx);

    /*Character*/
    } else if (ctx->stream->current == '\'') {
        ctx->token = tokenChar;
        lexerEatNext(ctx);

        while (   ctx->stream->current != '\''
               && ctx->stream->current != 0) {
            if (ctx->stream->current == '\\')
                lexerEatNext(ctx);

            lexerEatNext(ctx);
        }

        lexerEatNext(ctx);

    /*Other symbols or punctuation*/
    } else {
        ctx->token = tokenOther;
        lexerEatNext(ctx);

        /*Assume punctuation*/
        ctx->token = tokenPunct;

        switch (ctx->buffer[0]) {
        case '{': ctx->punct = punctLBrace; break;
        case '}': ctx->punct = punctRBrace; break;
        case '(': ctx->punct = punctLParen; break;
        case ')': ctx->punct = punctRParen; break;
        case '[': ctx->punct = punctLBracket; break;
        case ']': ctx->punct = punctRBracket; break;
        case ';': ctx->punct = punctSemicolon; break;
        case '.':
            ctx->punct = punctPeriod;

            if (ctx->stream->current == '.') {
                lexerEatNext(ctx);

                if (ctx->stream->current == '.') {
                    ctx->punct = punctEllipsis;
                    lexerEatNext(ctx);

                /*Oops, it's just two dots, backtrack*/
                } else {
                    streamPrev(ctx->stream);
                    ctx->length--;
                }
            }

            break;

        case ',': ctx->punct = punctComma; break;

        case '=': ctx->punct = lexerTryEatNext(ctx, '=') ? punctEqual : punctAssign; break;
        case '!': ctx->punct = lexerTryEatNext(ctx, '=') ? punctNotEqual : punctLogicalNot; break;
        case '>': ctx->punct = lexerTryEatNext(ctx, '=') ? punctGreaterEqual :
                               lexerTryEatNext(ctx, '>') ?
                                   (  lexerTryEatNext(ctx, '=')
                                    ? punctShrAssign
                                    : punctShr) :
                               punctGreater; break;
        case '<': ctx->punct = lexerTryEatNext(ctx, '=') ? punctLessEqual :
                               lexerTryEatNext(ctx, '<') ?
                                   (  lexerTryEatNext(ctx, '=')
                                    ? punctShlAssign
                                    : punctShl) :
                               punctLess; break;

        case '?': ctx->punct = punctQuestion; break;
        case ':': ctx->punct = punctColon; break;

        case '&': ctx->punct = lexerTryEatNext(ctx, '=') ? punctBitwiseAndAssign :
                               lexerTryEatNext(ctx, '&') ? punctLogicalAnd : punctBitwiseAnd; break;
        case '|': ctx->punct = lexerTryEatNext(ctx, '=') ? punctBitwiseOrAssign :
                               lexerTryEatNext(ctx, '|') ? punctLogicalOr : punctBitwiseOr; break;
        case '^': ctx->punct = lexerTryEatNext(ctx, '=') ? punctBitwiseXorAssign : punctBitwiseXor; break;
        case '~': ctx->punct = punctBitwiseNot;

        case '+': ctx->punct = lexerTryEatNext(ctx, '=') ? punctPlusAssign :
                               lexerTryEatNext(ctx, '+') ? punctPlusPlus : punctPlus; break;
        case '-': ctx->punct = lexerTryEatNext(ctx, '=') ? punctMinusAssign :
                               lexerTryEatNext(ctx, '-') ? punctMinusMinus :
                               lexerTryEatNext(ctx, '>') ? punctArrow : punctMinus; break;
        case '*': ctx->punct = lexerTryEatNext(ctx, '=') ? punctTimesAssign : punctTimes; break;
        case '/': ctx->punct = lexerTryEatNext(ctx, '=') ? punctDivideAssign : punctDivide; break;
        case '%': ctx->punct = lexerTryEatNext(ctx, '=') ? punctModuloAssign : punctModulo; break;

        /*Oops, actually an unrecognised character*/
        default: ctx->token = tokenOther;
        }
    }

    lexerEat(ctx, 0);

    //printf("token(%d:%d): '%s'.\n", ctx->stream->line, ctx->stream->lineChar, ctx->buffer);
}

static keywordTag lookKeyword (const char* str) {
    /*Manual trie
      Yeah it's ugly, but it's fast. And lexing is the slowest part of compilation.*/

    const char* rest[] = {str+1, str+2, str+3, str+4};

    switch (str[0]) {
    case 'a': return !strcmp(rest[0], "uto") ? keywordAuto : keywordUndefined;
    case 'd': return !strcmp(rest[0], "o") ? keywordDo : keywordUndefined;
    case 'r': return !strcmp(rest[0], "eturn") ? keywordReturn : keywordUndefined;
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

    case 'u':
        switch (str[1]) {
        case 'n': return !strcmp(rest[1], "ion") ? keywordUnion : keywordUndefined;
        case 's': return !strcmp(rest[1], "ing") ? keywordUsing : keywordUndefined;
        default: return keywordUndefined;
        }

    default: return keywordUndefined;
    }
}

const char* keywordTagGetStr (keywordTag tag) {
    if (tag == keywordUndefined) return "<undefined>";
    else if (tag == keywordUsing) return "using";
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

const char* punctTagGetStr (punctTag tag) {
    if (tag == punctUndefined) return "<undefined>";
    else if (tag == punctLBrace) return "{";
    else if (tag == punctRBrace) return "}";
    else if (tag == punctLParen) return "(";
    else if (tag == punctRParen) return ")";
    else if (tag == punctLBracket) return "[";
    else if (tag == punctRBracket) return "]";
    else if (tag == punctSemicolon) return ";";
    else if (tag == punctPeriod) return ".";
    else if (tag == punctEllipsis) return "...";
    else if (tag == punctComma) return ",";
    else if (tag == punctAssign) return "=";
    else if (tag == punctEqual) return "==";
    else if (tag == punctLogicalNot) return "!";
    else if (tag == punctNotEqual) return "!=";
    else if (tag == punctGreater) return ">";
    else if (tag == punctGreaterEqual) return ">=";
    else if (tag == punctShr) return ">>";
    else if (tag == punctShrAssign) return ">>=";
    else if (tag == punctLess) return "<";
    else if (tag == punctLessEqual) return "<=";
    else if (tag == punctShl) return "<<";
    else if (tag == punctShlAssign) return "<<=";
    else if (tag == punctQuestion) return "?";
    else if (tag == punctColon) return ":";
    else if (tag == punctBitwiseAnd) return "&";
    else if (tag == punctBitwiseAndAssign) return "&=";
    else if (tag == punctLogicalAnd) return "&&";
    else if (tag == punctBitwiseOr) return "|";
    else if (tag == punctBitwiseOrAssign) return "|=";
    else if (tag == punctLogicalOr) return "||";
    else if (tag == punctBitwiseXor) return "^";
    else if (tag == punctBitwiseXorAssign) return "^=";
    else if (tag == punctBitwiseNot) return "~";
    else if (tag == punctPlus) return "+";
    else if (tag == punctPlusAssign) return "+=";
    else if (tag == punctPlusPlus) return "++";
    else if (tag == punctMinus) return "-";
    else if (tag == punctMinusAssign) return "-=";
    else if (tag == punctMinusMinus) return "--";
    else if (tag == punctArrow) return "->";
    else if (tag == punctTimes) return "*";
    else if (tag == punctTimesAssign) return "*=";
    else if (tag == punctDivide) return "/";
    else if (tag == punctDivideAssign) return "/=";
    else if (tag == punctModulo) return "%";
    else if (tag == punctModuloAssign) return "%=";
    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("punctTagGetStr", "punctuation tag", str);
        free(str);
        return "<unhandled>";
    }
}
