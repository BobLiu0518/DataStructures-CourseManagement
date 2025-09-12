#define _WIN32_WINNT 0x0500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <windows.h>
#include <stdarg.h>
#include "033.h"

void pause() {
    HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(hConsoleInput, &prev_mode);
    SetConsoleMode(hConsoleInput, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT);

    INPUT_RECORD record;
    DWORD num_read;

    while (1) {
        WaitForSingleObject(hConsoleInput, INFINITE);
        ReadConsoleInput(hConsoleInput, &record, 1, &num_read);

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            break;
        } else if (record.EventType == MOUSE_EVENT && record.Event.MouseEvent.dwEventFlags == 0 && record.Event.MouseEvent.dwButtonState == 0) {
            break;
        }
    }

    SetConsoleMode(hConsoleInput, prev_mode);
}

void displayTitle(char* title) {
    int i = 0;
    printf("\033[7m");
    while (i < 40) {
        if (i == (40 - (int)strlen(title)) / 2 - 1) {
            printf("%s", title);
            i += strlen(title);
        } else {
            printf(" ");
            i++;
        }
    }
    printf("\033[0m\n");
    return;
}

int displayInput(char* prompt, char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("%s: \033[90m\033[?25h", prompt);
    fflush(stdin);
    int i = vscanf(format, args);
    printf("\033[0m\033[?25l");
    fflush(stdin);
    va_end(args);
    return i;
}

int displayInputMultiline(char* prompt, char* str, int max) {
    int length = 0;
    char* tmp = calloc(max, sizeof(char));
    str[0] = '\0';
    printf("%s:\n\033[?25h", prompt);
    fflush(stdin);

    do {
        printf("> \033[90m");
        fgets(tmp, max, stdin);
        printf("\033[0m");
        if (length + (int)strlen(tmp) > max) {
            printf(Yellow("警告：")"已经超出长度限制，输入内容会被截断。\n");
            break;
        }
        length += strlen(tmp);
        strcat(str, tmp);
    } while (tmp[0] != '\n' && tmp[0] != '\r');
    while (length > 0 && (str[length - 1] == '\n' || str[length - 1] == '\r')) {
        str[length - 1] = '\0';
        length--;
    }
    free(tmp);

    fflush(stdin);
    printf("\n\033[?25l");
    return length;
}

int displayInputPassword(char* prompt, char* result, int max) {
    int i;
    char c;
    printf("%s: \033[90m\033[?25h", prompt);
    fflush(stdin);
    i = 0;
    while (1) {
        c = getch();
        if (c == '\b') {
            if (i > 0) {
                result[i] = '\0';
                i--;
                printf("\b \b");
            } else {
                printf("\a");
            }
        } else {
            if (isprint(c)) {
                if (i >= max - 1) {
                    printf("\a");
                } else {
                    result[i] = c;
                    i++;
                    printf("*");
                }
            } else {
                result[i] = '\0';
                i++;
                break;
            }
        }
    }
    fflush(stdin);
    printf("\033[0m\n\033[?25l");
    return i;
}

int displaySelect(char* title, char* prompt, int count, char** options) {
    int i, j, selection = 0;
    HANDLE hConsoleInput;
    DWORD prev_mode;
    INPUT_RECORD record;
    DWORD num_read;
    int prompt_lines = (title ? 1 : 0) + (prompt ? 1 : 0);

    if (!count) {
        printf("无可选项\n");
        pause();
        return -1;
    }

    hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hConsoleInput, &prev_mode);
    SetConsoleMode(hConsoleInput, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT);

    printf("\033[2J"); // 清屏
    while (1) {
        printf("\033[1;1H"); // 重置光标位置

        if (title) {
            displayTitle(title);
        }

        // 显示 prompt
        if (prompt) {
            printf("%s\n", prompt);
        }

        // 显示选项
        for (i = 0; i < count; i++) {
            if (selection == i) {
                printf("\033[37;100m"); // 反色
            }
            printf(" > %s", options[i]);
            for (j = strlen(options[i]); j < 37; j++) {
                printf(" ");
            }
            printf("\033[0m\n"); // 重置输出样式
        }

        // 处理输入
        WaitForSingleObject(hConsoleInput, INFINITE);
        ReadConsoleInput(hConsoleInput, &record, 1, &num_read);

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            int vkey = record.Event.KeyEvent.wVirtualKeyCode;
            char c = record.Event.KeyEvent.uChar.AsciiChar;

            if (c >= '1' && c <= '9' && c - '1' < count) {
                if (selection == c - '1') {
                    break;
                } else {
                    selection = c - '1';
                }
            } else if (vkey == VK_UP || c == 'w' || c == 'W') {
                selection = (selection + count - 1) % count;
            } else if (vkey == VK_DOWN || vkey == VK_TAB || c == 's' || c == 'S') {
                selection = (selection + 1) % count;
            } else if (vkey == VK_ESCAPE) {
                selection = -1;
                break;
            } else if (vkey == VK_RETURN || vkey == VK_SPACE) {
                break;
            }
        } else if (record.EventType == MOUSE_EVENT) {
            int mouseY = record.Event.MouseEvent.dwMousePosition.Y;
            int new_selection = mouseY - prompt_lines;

            if (new_selection >= 0 && new_selection < count) {
                selection = new_selection;

                if (record.Event.MouseEvent.dwEventFlags == 0 && (record.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) == 0) {
                    break;
                }
            }
        }
    }

    printf("\033[1;1H\033[2J"); // 重置光标位置 & 清屏
    SetConsoleMode(hConsoleInput, prev_mode); // 恢复控制台模式
    fflush(stdin);
    return selection;
}