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
    system("title �����רҵ�γ̹���ϵͳ");
    printf("\033[?25l");
    srand((unsigned)time(nullptr));
    CreateDirectory("storage", nullptr);
    while (1) {
        switch (displaySelect("�����רҵ�γ̹���ϵͳ", nullptr, 3 + DEBUG, OPTIONS("��¼", "��������", "�˳�", "����"))) {
        case 0:
            int username;
            char password[10];
            displayTitle("��¼");
            displayInput("ѧ����", "%d", &username);
            displayInputPassword("����", password, 10);
            break;
        case 1:
            printDebug("������Ϣ");
            break;
        case -1:
        case 2:
        default:
            displayTitle("�����רҵ�γ̹���ϵͳ");
            printf("�ټ���\n");
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
                    printDebug("[%02d] ���Բ��� %d", i + 1, data[i]);
                    if (insertRecord(tree, data[i], data + i)) {
                        sprintf(buffer, "storage\\test%02d.html", i + 1);
                        saveTreeMermaid(tree, buffer, false);
                        checkTreeLegitimacy(tree, true);
                    } else {
                        printDebug("����ʧ�ܣ��ظ���");
                        i--;
                    }
                }
                for (int i = 50; i < 70; i++) {
                    int remove = data[rand() % 50];
                    printDebug("[%02d] ����ɾ�� %d", i + 1, remove);
                    if (removeRecord(tree, remove)) {
                        sprintf(buffer, "storage\\test%02d.html", i + 1);
                        saveTreeMermaid(tree, buffer, false);
                        checkTreeLegitimacy(tree, true);
                    } else {
                        printDebug("ɾ��ʧ�ܣ�δ�ҵ���");
                        i--;
                    }
                }
                printDebug("�������ٲ�������");
                destroyTree(tree);
                printDebug("��Ү��ȫ����ɣ�");
            }
        }
        pause();
    }

    return 0;
}