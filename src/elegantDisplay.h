#ifndef ELEGANT_DISPLAY_H
#define ELEGANT_DISPLAY_H

#include "debug.h"

#define STR_ARRAY(...) ((char* []) { __VA_ARGS__ })
#define OPTIONS(...) (sizeof(STR_ARRAY(__VA_ARGS__)) / sizeof(char*)), STR_ARRAY(__VA_ARGS__)
#define OPTIONS_WITH_DEBUG(...) (sizeof(STR_ARRAY(__VA_ARGS__)) / sizeof(char*)) - !DEBUG, STR_ARRAY(__VA_ARGS__)

void pause();
void displayTitle(char* title);
int displayInput(char* prompt, char* format, ...);
int displayInputMultiline(char* prompt, char* str, int max);
int displayInputPassword(char* prompt, char* result, int max);
int displaySelect(char* title, char* prompt, int count, char** options);

#endif