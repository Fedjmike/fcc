typedef struct {
    int errors, warnings;
} compilerResult;

compilerResult compiler (const char* input, const char* output);
