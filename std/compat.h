#define CONCATIMPL(x, y) x##y
#define CONCAT(x, y) CONCATIMPL(x, y)
#define USINGIMPL(var) static const char* var = 0 ? (var = 0) :
#define using USINGIMPL(CONCAT(__usingcompatstr_, __COUNTER__))

typedef enum {false = 0, true = 1} bool;
