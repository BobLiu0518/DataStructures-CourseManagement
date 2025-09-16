#include <string.h>
#include "033.h"
#include "data.h"
#include "bPlusTree.h"
#include "elegantDisplay.h"

void saveData(char* filename, BPTree* tree, size_t size) {
    int count = 0;
    BPTNode* node = tree->head;
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf(Red("´íÎó£º")"±£´æÊý¾ÝÊ§°Ü¡£\n");
        pause();
        return;
    }

    fwrite(&count, sizeof(int), 1, fp);
    while (node) {
        for (int i = 0; i < node->keyCount; i++) {
            fwrite(node->records[i], size, 1, fp);
            count++;
        }
        node = node->next;
    }
    fseek(fp, 0, SEEK_SET);
    fwrite(&count, sizeof(int), 1, fp);

    fclose(fp);
}

int readData(char* filename, size_t size, void(*operation)(void*)) {
    int count = 0;
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return 0;
    }

    fread(&count, sizeof(int), 1, fp);
    for (int i = 0; i < count; i++) {
        void* data = malloc(size);
        fread(data, size, 1, fp);
        operation(data);
    }

    fclose(fp);
    return count;
}

int verifyPassword(char* password, char* hash) {
    return crypto_pwhash_str_verify(hash, password, strlen(password));
}

int setPassword(char* password, char* hash) {
    return crypto_pwhash_str(hash, password, strlen(password), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE);
}