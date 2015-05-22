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

lexerCtx* lexerInit (const char* filename) {
    lexerCtx* ctx = malloc(sizeof(lexerCtx));
    ctx->stream = streamInit(filename);
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

    /*String/character*/
    } else if (   ctx->stream->current == '"'
               || ctx->stream->current == '\'') {
        ctx->token = ctx->stream->current == '"' ? tokenStr : tokenChar;
        streamNext(ctx->stream);

        while (   ctx->stream->current != (ctx->token == tokenStr ? '"' : '\'')
               && ctx->stream->current != 0) {
            if (ctx->stream->current == '\\')
                lexerEatNext(ctx);

            lexerEatNext(ctx);
        }

        streamNext(ctx->stream);

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
        case '~': ctx->punct = punctBitwiseNot; break;

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

static keywordTag keywordMatch (const char* str, int n, const char* look, keywordTag kw) {
    return !strcmp(str+n+1, look+n+1) ? kw : keywordUndefined;
}

static keywordTag keywordMatch2 (const char* str, int n, const char* look, keywordTag kw,
                                 const char* look2, keywordTag kw2) {
    return   !strcmp(str+n+1, look+n+1) ? kw
           : !strcmp(str+n+1, look2+n+1) ? kw2 : keywordUndefined;
}

static keywordTag lookKeyword (const char* str) {
    /*Manual trie
      Yeah it's ugly, but it's fast. And lexing is the slowest part of compilation.*/

    const char* rest[] = {str+1, str+2, str+3, str+4};

    switch (str[0]) {
    case 'd': return keywordMatch(str, 0, "do", keywordDo);
    case 'r': return keywordMatch(str, 0, "return", keywordReturn);
    case 'w': return keywordMatch(str, 0, "while", keywordWhile);

    case 'a': return keywordMatch2(str, 0, "assert", keywordAssert,
                                           "auto", keywordAuto);
    case 'b': return keywordMatch2(str, 0, "bool", keywordBool,
                                           "break", keywordBreak);
    case 't': return keywordMatch2(str, 0, "true", keywordTrue,
                                           "typedef", keywordTypedef);
    case 'u': return keywordMatch2(str, 0, "union", keywordUnion,
                                           "using", keywordUsing);

    case 'c':
        switch (str[1]) {
        case 'h': return keywordMatch(str, 1, "char", keywordChar);
        case 'o':
            if (str[2] != 'n')
                return keywordUndefined;

            return keywordMatch2(str, 2, "const", keywordConst,
                                         "continue", keywordContinue);

        default: return keywordUndefined;
        }

    case 'e':
        switch (str[1]) {
        case 'l': return keywordMatch(str, 1, "else", keywordElse);
        case 'n': return keywordMatch(str, 1, "enum", keywordEnum);
        case 'x': return keywordMatch(str, 1, "extern", keywordExtern);
        default: return keywordUndefined;
        }

    case 'f': return keywordMatch2(str, 0, "false", keywordFalse,
                                           "for", keywordFor);

    case 'i': return keywordMatch2(str, 0, "if", keywordIf,
                                           "int", keywordInt);

    case 's':
        switch (str[1]) {
        case 'i': return keywordMatch(str, 1, "sizeof", keywordSizeof);
        case 't': return keywordMatch2(str, 1, "static", keywordStatic,
                                               "struct", keywordStruct);
        default: return keywordUndefined;
        }

    case 'v':
        switch (str[1]) {
        case 'a':
            if (str[2] == '_') {
                switch (str[3]) {
                    case 'a': return keywordMatch(str, 3, "va_arg", keywordVAArg);
                    case 'c': return keywordMatch(str, 3, "va_copy", keywordVACopy);
                    case 'e': return keywordMatch(str, 3, "va_end", keywordVAEnd);
                    case 's': return keywordMatch(str, 3, "va_start", keywordVAStart);
                    default: return keywordUndefined;
                }

            } else
                return keywordUndefined;

        case 'o': return keywordMatch(str, 1, "void", keywordVoid);
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
    else if (tag == keywordVAStart) return "va_start";
    else if (tag == keywordVAEnd) return "va_end";
    else if (tag == keywordVAArg) return "va_arg";
    else if (tag == keywordVACopy) return "va_copy";
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
