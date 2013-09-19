#include "../std/std.h"

struct ast;
struct architecture;

typedef struct evalResult {
    bool known;
    int value;
} evalResult;

evalResult eval (const struct architecture* arch, struct ast* Node);
