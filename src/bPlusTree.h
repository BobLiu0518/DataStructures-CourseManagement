#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H

#include <assert.h>

#ifndef B_PLUS_TREE_ORDER
#define B_PLUS_TREE_ORDER 5
#endif

static_assert(B_PLUS_TREE_ORDER >= 3, "Order must >= 3");

typedef unsigned long long Key;

typedef struct BPTNode {
    // 是否为叶节点
    bool isLeaf;
    // 父节点
    struct BPTNode* parent;
    // 节点大小
    int keyCount;
    // 关键字
    Key keys[B_PLUS_TREE_ORDER];

    union {
        // 对于内部节点
        struct BPTNode* children[B_PLUS_TREE_ORDER + 1];
        // 对于叶节点
        struct {
            // 记录
            void* records[B_PLUS_TREE_ORDER];
            // 指向下一个
            struct BPTNode* next;
        };
        // 便于统一访问
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

BPTree* createTree(void (*freeRecord)(void*), bool allowDuplicateKey);
void destroyTree(BPTree* tree);
bool insertRecord(BPTree* tree, Key key, void* record);
bool removeRecord(BPTree* tree, Key key);
bool replaceRecord(BPTree* tree, Key key, void* record);
void* findRecord(BPTree* tree, Key key);
void saveTreeMermaid(BPTree* tree, char* filename, bool display);
void checkTreeLegitimacy(BPTree* tree, bool recordIsKey);

#define DECLARE_B_PLUS_TREE_FUNC(Type) \
    BPTree* createTree##Type(void (*freeRecord)(Type*), bool allowDuplicateKey) { return createTree((void (*)(void*))freeRecord, allowDuplicateKey); }

#endif