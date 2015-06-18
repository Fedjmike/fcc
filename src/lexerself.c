#include "../inc/lexer.h"

#include "../std/std.h"
#include "../inc/debug.h"

#include "../inc/stream.h"

#include "stdlib.h"
#include "string.h"
#include "ctype.h"

using "../inc/lexer.h";

using "../std/std.h";
using "../inc/debug.h";

using "../inc/stream.h";

using "stdlib.h";
using "string.h";
using "ctype.h";

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
    lexerEat(ctx, (int) streamNext(ctx->stream));
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

        enum {tableSize = sizeof(ctx->buffer[0])*256};
        typedef punctTag (*charHandler)(lexerCtx*);
        charHandler table[tableSize];
        memset(table, 0, sizeof(table));

        table['{'] = [](lexerCtx*) (punctLBrace);
        table['}'] = [](lexerCtx*) (punctRBrace);
        table['('] = [](lexerCtx*) (punctLParen);
        table[')'] = [](lexerCtx*) (punctRParen);
        table['['] = [](lexerCtx*) (punctLBracket);
        table[']'] = [](lexerCtx*) (punctRBracket);
        table[';'] = [](lexerCtx*) (punctSemicolon);
        table['.'] = [](lexerCtx* ctx) {
            if (lexerTryEatNext(ctx, '.')) {
                if (lexerTryEatNext(ctx, '.'))
                    return punctEllipsis;

                /*Oops, it's just two dots, backtrack*/
                else {
                    streamPrev(ctx->stream);
                    ctx->length--;
                }
            }

            return punctPeriod;
        };

        table[','] = [](lexerCtx*) (punctComma);

        table['='] = [](lexerCtx* ctx) (lexerTryEatNext(ctx, '=') ? punctEqual : punctAssign);
        table['!'] = [](lexerCtx* ctx) (lexerTryEatNext(ctx, '=') ? punctNotEqual : punctLogicalNot);
        table['>'] = [](lexerCtx* ctx) (
              lexerTryEatNext(ctx, '=') ? punctGreaterEqual
            : lexerTryEatNext(ctx, '>') ? (  lexerTryEatNext(ctx, '=') ? punctShrAssign
                                           : punctShr)
            : punctGreater
        );
        table['<'] = [](lexerCtx* ctx) (
              lexerTryEatNext(ctx, '=') ? punctLessEqual
            : lexerTryEatNext(ctx, '<') ? (  lexerTryEatNext(ctx, '=') ? punctShlAssign
                                           : punctShl)
            : punctLess
        );

        table['?'] = [](lexerCtx*) (punctQuestion);
        table[':'] = [](lexerCtx*) (punctColon);

        table['&'] = [](lexerCtx* ctx) (
            lexerTryEatNext(ctx, '=') ? punctBitwiseAndAssign :
            lexerTryEatNext(ctx, '&') ? punctLogicalAnd : punctBitwiseAnd
        );
        table['|'] = [](lexerCtx* ctx) (
            lexerTryEatNext(ctx, '=') ? punctBitwiseOrAssign :
            lexerTryEatNext(ctx, '|') ? punctLogicalOr : punctBitwiseOr
        );
        table['^'] = [](lexerCtx* ctx) (lexerTryEatNext(ctx, '=') ? punctBitwiseXorAssign : punctBitwiseXor);
        table['~'] = [](lexerCtx*) (punctBitwiseNot);

        table['+'] = [](lexerCtx* ctx) (
            lexerTryEatNext(ctx, '=') ? punctPlusAssign :
            lexerTryEatNext(ctx, '+') ? punctPlusPlus : punctPlus
        );
        table['-'] = [](lexerCtx* ctx) (
            lexerTryEatNext(ctx, '=') ? punctMinusAssign :
            lexerTryEatNext(ctx, '-') ? punctMinusMinus :
            lexerTryEatNext(ctx, '>') ? punctArrow : punctMinus
        );
        table['*'] = [](lexerCtx* ctx) (lexerTryEatNext(ctx, '=') ? punctTimesAssign : punctTimes);
        table['/'] = [](lexerCtx* ctx) (lexerTryEatNext(ctx, '=') ? punctDivideAssign : punctDivide);
        table['%'] = [](lexerCtx* ctx) (lexerTryEatNext(ctx, '=') ? punctModuloAssign : punctModulo);

        charHandler fn = table[(int) ctx->buffer[0]];

        if (fn != 0)
            ctx->punct = fn(ctx);

        else
            ctx->punct = punctUndefined;
    }

    lexerEat(ctx, 0);

    //printf("token(%d:%d): '%s'.\n", ctx->stream->line, ctx->stream->lineChar, ctx->buffer);
}

static keywordTag lookKeyword (const char* str) {
    /*Manual trie
      Yeah it's ugly, but it's fast. And lexing is the slowest part of compilation.*/

    enum {tableSize = sizeof(str[0])};
    keywordTag (*table[tableSize*256])(const char*);
    memset(table, 0, sizeof(table));

    table['a'] = [](const char* str) (!strcmp(str+1, "uto") ? keywordAuto : keywordUndefined);
    table['d'] = [](const char* str) (!strcmp(str+1, "o") ? keywordDo : keywordUndefined);
    table['r'] = [](const char* str) (!strcmp(str+1, "eturn") ? keywordReturn : keywordUndefined);
    table['w'] = [](const char* str) (!strcmp(str+1, "hile") ? keywordWhile : keywordUndefined);

    table['b'] = [](const char* str) {
        if (str[1] == 'o')
            return !strcmp(str+2, "ol") ? keywordBool : keywordUndefined;

        else
            return !strcmp(str+1, "reak") ? keywordBreak : keywordUndefined;
    };

    table['c'] = [](const char* str) {
        if (str[1] == 'o') {
            if (str[2] == 'n') {
                if (str[3] == 's')
                    return !strcmp(str+4, "t") ? keywordConst : keywordUndefined;

                else
                    return !strcmp(str+3, "tinue") ? keywordContinue : keywordUndefined;

            } else
                return keywordUndefined;

        } else
            return !strcmp(str+1, "har") ? keywordChar : keywordUndefined;
    };

    table['e'] = [](const char* str) {
        if (str[1] == 'l')
            return !strcmp(str+2, "se") ? keywordElse : keywordUndefined;

        else if (str[1] == 'n')
            return !strcmp(str+2, "um") ? keywordEnum : keywordUndefined;

        else
            return !strcmp(str+1, "xtern") ? keywordExtern : keywordUndefined;
    };

    table['f'] = [](const char* str) {
        if (str[1] == 'a')
            return !strcmp(str+2, "lse") ? keywordFalse : keywordUndefined;

        else
            return !strcmp(str+1, "or") ? keywordFor : keywordUndefined;
    };

    table['i'] = [](const char* str) {
        if (str[1] == 'f')
            return str[2] == 0 ? keywordIf : keywordUndefined;

        else
            return !strcmp(str+1, "nt") ? keywordInt : keywordUndefined;
    };

    table['s'] = [](const char* str) {
        if (str[1] == 't') {
            if (str[2] == 'a')
                return !strcmp(str+3, "tic") ? keywordStatic : keywordUndefined;

            else
                return !strcmp(str+2, "ruct") ? keywordStruct : keywordUndefined;

        } else
            return !strcmp(str+1, "izeof") ? keywordSizeof : keywordUndefined;
    };

    table['t'] = [](const char* str) {
        if (str[1] == 'r')
            return !strcmp(str+2, "ue") ? keywordTrue : keywordUndefined;

        else
            return !strcmp(str+1, "ypedef") ? keywordTypedef : keywordUndefined;
    };

    table['u'] = [](const char* str) {
        if (str[1] == 'n')
            return !strcmp(str+2, "ion") ? keywordUnion : keywordUndefined;

        else
            return !strcmp(str+1, "sing") ? keywordUsing : keywordUndefined;
    };

    table['v'] = [](const char* str) {
        if (str[1] == 'a') {
            if (str[2] == '_') {
                if (str[3] == 'a')
                    return !strcmp(str+4, "rg") ? keywordVAArg : keywordUndefined;

                else if (str[3] == 'c')
                    return !strcmp(str+4, "opy") ? keywordVACopy : keywordUndefined;

                else if (str[3] == 'e')
                    return !strcmp(str+4, "nd") ? keywordVAEnd : keywordUndefined;

                else
                    return !strcmp(str+3, "start") ? keywordVAStart : keywordUndefined;

            } else
                return keywordUndefined;

        } else
            return !strcmp(str+1, "oid") ? keywordVoid : keywordUndefined;
    };

    if (table[(int) str[0]])
        return table[(int) str[0]](str);

    else
        return keywordUndefined;
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
