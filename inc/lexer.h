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
} tokenClass;

typedef struct  {
    streamCtx* stream;

    tokenClass token;
    char* buffer;
    int bufferSize;
    int length;
} lexerCtx;

lexerCtx* lexerInit (const char* File);
void lexerEnd (lexerCtx* ctx);

void lexerNext (lexerCtx* ctx);
