#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "debug.h"
#include "033.h"
#include "elegantDisplay.h"
#include "bPlusTree.h"
#include "data.h"
DECLARE_B_PLUS_TREE_FUNC(User);

int main() {
    system("title 计算机专业课程管理系统");
    printf("\033[?25l");
    srand((unsigned)time(nullptr));
    CreateDirectory("storage", nullptr);
    while (1) {
        switch (displaySelect("计算机专业课程管理系统", nullptr, 3 + DEBUG, OPTIONS("登录", "忘记密码", "退出", "调试"))) {
        case 0:
            int username;
            char password[10];
            displayTitle("登录");
            displayInput("学工号", "%d", &username);
            displayInputPassword("密码", password, 10);
            break;
        case 1:
            printDebug("调试信息");
            break;
        case -1:
        case 2:
        default:
            displayTitle("计算机专业课程管理系统");
            printf("再见！\n");
            pause();
            exit(0);
            break;
        case 3:
            while (true) {
                Key data[50];
                char buffer[50];
                BPTree* tree = createTree(nullptr);
                for (int i = 0; i < 50; i++) {
                    data[i] = rand() % 100;
                    printDebug("[%02d] 尝试插入 %d", i + 1, data[i]);
                    if (insertRecord(tree, data[i], data + i)) {
                        sprintf(buffer, "storage\\test%02d.html", i + 1);
                        saveTreeMermaid(tree, buffer, false);
                        checkTreeLegitimacy(tree, true);
                    } else {
                        printDebug("插入失败：重复。");
                        i--;
                    }
                }
                for (int i = 50; i < 70; i++) {
                    int remove = data[rand() % 50];
                    printDebug("[%02d] 尝试删除 %d", i + 1, remove);
                    if (removeRecord(tree, remove)) {
                        sprintf(buffer, "storage\\test%02d.html", i + 1);
                        saveTreeMermaid(tree, buffer, false);
                        checkTreeLegitimacy(tree, true);
                    } else {
                        printDebug("删除失败：未找到。");
                        i--;
                    }
                }
                printDebug("正在销毁测试树…");
                destroyTree(tree);
                printDebug("好耶！全部完成！");
            }
        }
        pause();
    }

    return 0;
}