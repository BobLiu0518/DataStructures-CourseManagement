#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H

#include <assert.h>

#ifndef B_PLUS_TREE_ORDER
#define B_PLUS_TREE_ORDER 5
#endif

static_assert(B_PLUS_TREE_ORDER >= 3, "Order must >= 3");

typedef unsigned long long Key;

typedef struct BPTNode {
    // �Ƿ�ΪҶ�ڵ�
    bool isLeaf;
    // ���ڵ�
    struct BPTNode* parent;
    // �ڵ��С
    int keyCount;
    // �ؼ���
    Key keys[B_PLUS_TREE_ORDER];

    union {
        // �����ڲ��ڵ�
        struct BPTNode* children[B_PLUS_TREE_ORDER + 1];
        // ����Ҷ�ڵ�
        struct {
            // ��¼
            void* records[B_PLUS_TREE_ORDER];
            // ָ����һ��
            struct BPTNode* next;
        };
        // ����ͳһ����
        struct {
            void* pointers[B_PLUS_TREE_ORDER + 1];
        };
    };
} BPTNode;

typedef struct BPTree {
    BPTNode* root;
    BPTNode* head;
    void (*freeRecord)(void*);
    bool allowDuplicateKey;
} BPTree;

typedef  struct {
    void** arr;
    int total;
    int capacity;
} RecordArray;

BPTree* createTree(void (*freeRecord)(void*), bool allowDuplicateKey);
void destroyTree(BPTree* tree);
Key getBiggestKey(BPTree* tree);
int getTotalKeyCount(BPTree* tree);
bool insertRecord(BPTree* tree, Key key, void* record);
void* removeRecord(BPTree* tree, Key key, void* record);
bool replaceRecord(BPTree* tree, Key key, void* record);
void* findRecord(BPTree* tree, Key key);
void findRecordRange(BPTree* tree, Key min, Key max, void (*operation)(void*));
RecordArray findRecordRangeArray(BPTree* tree, Key min, Key max);
void saveTreeMermaid(BPTree* tree, char* filename, bool display);
void checkTreeLegitimacy(BPTree* tree, bool recordIsKey);

#define DECLARE_B_PLUS_TREE_FUNC(Type) \
    static inline BPTree* createTree##Type(void (*freeRecord)(Type*), bool allowDuplicateKey) { return createTree((void (*)(void*))freeRecord, allowDuplicateKey); } \
    static inline void findRecordRange##Type(BPTree* tree, Key min, Key max, void (*operation)(Type*)) { findRecordRange(tree, min, max, (void (*)(void*))operation); }

#endif