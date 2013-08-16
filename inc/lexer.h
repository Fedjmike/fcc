#pragma once

#include "stream.h"

typedef enum {
    tokenUndefined,
    tokenOther,
    tokenEOF,
    tokenKeyword,
    tokenIdent,
    tokenInt,
    tokenStr,
    tokenChar
} tokenTag;

typedef enum {
    keywordUndefined,
    keywordIf, keywordElse, keywordWhile, keywordDo, keywordFor,
    keywordReturn, keywordBreak, keywordContinue,
    keywordSizeof,
    keywordConst,
    keywordAuto, keywordStatic, keywordExtern, keywordTypedef,
    keywordStruct, keywordUnion, keywordEnum,
    keywordVoid, keywordBool, keywordChar, keywordInt,
    keywordTrue, keywordFalse
} keywordTag;

typedef struct {
    int line;
    int lineChar;
} tokenLocation;

typedef struct  {
    streamCtx* stream;

    tokenTag token;
    keywordTag keyword;
    char* buffer;
    int bufferSize;
    int length;
} lexerCtx;

lexerCtx* lexerInit (const char* File);
void lexerEnd (lexerCtx* ctx);

tokenLocation lexerNext (lexerCtx* ctx);

const char* keywordTagGetStr (keywordTag tag);
