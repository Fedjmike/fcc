using "_stdarg.h";

typedef _va_list va_list;

void va_start (va_list, ...);
void va_end (va_list);
void va_copy (va_list, va_list);