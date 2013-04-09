#pragma once

#include "stream.h"

typedef enum {
    tokenUndefined,
    tokenOther,
    tokenEOF,
    tokenIdent,
    tokenInt,
    tokenStr,
    tokenChar
} tokenTag;

typedef struct {
    int line;
    int lineChar;
} tokenLocation;

typedef struct  {
    streamCtx* stream;

    tokenTag token;
    char* buffer;
    int bufferSize;
    int length;
} lexerCtx;

lexerCtx* lexerInit (const char* File);
void lexerEnd (lexerCtx* ctx);

tokenLocation lexerNext (lexerCtx* ctx);
