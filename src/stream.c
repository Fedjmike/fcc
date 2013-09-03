#include "../inc/stream.h"

#include "../std/std.h"

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

streamCtx* streamInit (FILE* file) {
    streamCtx* ctx = malloc(sizeof(streamCtx));
    ctx->file = file;

    ctx->current = 0;
    ctx->line = 1;
    ctx->lineChar = 0;

    streamNext(ctx);

    return ctx;
}

void streamEnd (streamCtx* ctx) {
    fclose(ctx->file);
    free(ctx);
}

char streamNext (streamCtx* ctx) {
    char old = ctx->current;
    ctx->current = fgetc(ctx->file);

    if (feof(ctx->file) || ctx->current == ((char) 0xFF))
        ctx->current = 0;

    ctx->lineChar++;

    if (old == '\n') {
        ctx->line++;
        ctx->lineChar = 0;

    } else if (old == '\t')
        ctx->lineChar += 3;


    //printf("%d\t%c\n", (int) ctx->current, ctx->current);

    return old;
}

char streamPrev (streamCtx* ctx) {
    char old = ctx->current;

    fseek(ctx->file, -2, SEEK_CUR);
    streamNext(ctx);

    ctx->lineChar--;

    if (ctx->current == '\n') {
        ctx->line--;
        /*Since it would be such a hassle to go back and count chars
          (or keep track of them for all lines), we'll just start
          using Python indices (-1 = last char, -2 = second last etc)*/
        ctx->lineChar = -1;

    } else if (ctx->current == '\t')
        ctx->lineChar -= 3;

    return old;

}

