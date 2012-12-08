#pragma once

#include "lexer.h"

struct ast;
struct sym;

typedef struct {
    lexerCtx* lexer;

    struct sym* scope;

    bool insideLoop;

    int errors;
    int warnings;
} parserCtx;

struct ast* parser (char* File);
