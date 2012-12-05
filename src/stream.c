#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#include "../inc/stream.h"

/*16 defines the maximum level of modules you can import*/
FILE* streamFiles[16];
char* streamFilenames[16];
int streamMaxFiles = 16;
int streamFileNo = 0;

char* streamFilename;
char streamChar = 0;

void streamInit (char* File) {
    streamPush(File);
}

void streamEnd () {
    while (streamFileNo)
        streamPop();
}

void streamPush (char* File) {
    streamFilenames[streamFileNo] = strdup(File);
    streamFiles[streamFileNo] = fopen(File, "r");

    if (streamFiles[streamFileNo] == 0) {
        printf("Error opening file, '%s'.\n", File);
        exit(EXIT_FAILURE);
    }

    streamFileNo++;

    streamNext();
}

void streamPop () {
    streamFileNo--;

    if (streamFiles[streamFileNo])
        fclose(streamFiles[streamFileNo]);

    free(streamFilenames[streamFileNo]);
}

char streamNext () {
    char Old = streamChar;
    streamChar = fgetc(streamFiles[streamFileNo-1]);

    if (feof(streamFiles[streamFileNo-1]) || streamChar == ((char) 0xFF))
        streamChar = 0;

    //printf("%d\t%c\n", (int) streamChar, streamChar);

    return Old;
}

char streamPrev () {
    char Old = streamChar;

    fseek(streamFiles[streamFileNo-1], -2, SEEK_CUR);
    streamNext();

    return Old;
}

int streamGetPos () {
    return ftell(streamFiles[streamFileNo-1]);
}
