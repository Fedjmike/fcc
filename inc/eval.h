#include "../std/std.h"

typedef struct ast ast;
typedef struct architecture architecture;

typedef struct evalResult {
    bool known;
    int value;
} evalResult;

evalResult eval (const architecture* arch, ast* Node);
