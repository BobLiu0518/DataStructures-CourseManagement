#ifndef DEBUG_H
#define DEBUG_H

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG == 1

#include <stdio.h>
#include <stdarg.h>
static inline void printDebug(char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[33m[DEBUG] ");
    vprintf(format, args);
    printf("\033[0m\n");
    va_end(args);
}
static inline void printFatal(char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[31m[FATAL] ");
    vprintf(format, args);
    printf("\033[0m\n");
    va_end(args);
    exit(1);
}
#else

// void printDebug(char* format [[maybe_unused]], ...) { }
#define printDebug(...) ((void)0)
#define printFatal(...) ((void)0)

#endif

#endif