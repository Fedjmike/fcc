#include "../std/std.h"

using "forward.h";

typedef struct ast ast;
typedef struct architecture architecture;

typedef struct evalResult {
    bool known;
    int value;
} evalResult;

evalResult eval (const architecture* arch, const ast* Node);

bool evalIsConstantInit (const ast* Node);
