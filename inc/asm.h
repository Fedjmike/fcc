#pragma once

#include "stdarg.h"
#include "stdio.h"

void asmInit (FILE* File);
void asmOut (char* format, ...);
void asmVarOut (char* format, va_list args);
void asmComment (char* format, ...);
void asmEnter ();
void asmLeave ();
