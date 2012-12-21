#pragma once

#include "stream.h"

typedef enum {
    tokenUndefined,
    tokenOther,
    tokenEOF,
    tokenIdent,
    tokenInt
} tokenClass;

typedef struct  {
    streamCtx* stream;

    tokenClass token;
    char* buffer;
    int bufferSize;
} lexerCtx;

lexerCtx* lexerInit (char* File);
void lexerEnd (lexerCtx* ctx);

void lexerNext (lexerCtx* ctx);
