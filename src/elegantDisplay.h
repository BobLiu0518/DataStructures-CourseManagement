#ifndef ELEGANT_DISPLAY_H
#define ELEGANT_DISPLAY_H

#define OPTIONS(...) ((char* []) { __VA_ARGS__ })

void pause();
void displayTitle(char* title);
int displayInput(char* prompt, char* format, ...);
int displayInputMultiline(char* prompt, char* str, int max);
int displayInputPassword(char* prompt, char* result, int max);
int displaySelect(char* title, char* prompt, int count, char** options);

#endif