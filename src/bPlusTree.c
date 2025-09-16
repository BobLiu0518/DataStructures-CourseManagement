//                    _ooOoo_
//                   o8888888o
//                   88" . "88
//                   (| -_- |)
//                    O\ = /O
//                ____/`---'\____
//              .   ' \\| |// `.
//               / \\||| : |||// \
//             / _||||| -:- |||||- \
//               | | \\\ - /// | |
//             | \_| ''\---/'' | |
//              \ .-\__ `-` ___/-. /
//           ___`. .' /--.--\ `. . __
//        ."" '< `.___\_<|>_/___.' >'"".
//       | | : `- \`.;`\ _ /`;.`/ - ` : | |
//         \ \ `-. \_ __\ /__ _/ .-` / /
// ======`-.____`-.___\_____/___.-`____.-'======
//                    `=---='

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "bPlusTree.h"
#include "debug.h"

typedef enum { TRAVERSE_PREORDER, TRAVERSE_POSTORDER } TraverseOrder;

void traverseLeaf(BPTree* tree, void(*func)(BPTNode*, va_list), ...) {
    va_list args;
    va_start(args, func);
    BPTNode* p = tree->head;
    while (p) {
        func(p, args);
        p = p->next;
    }
    va_end(args);
}

void traverseTreeNode(BPTNode* node, TraverseOrder order, void(*func)(BPTNode*, va_list), va_list args) {
    if (order == TRAVERSE_PREORDER) {
        func(node, args);
    }
    if (!node->isLeaf) {
        for (int i = 0; i <= node->keyCount; i++) {
            traverseTreeNode(node->children[i], order, func, args);
        }
    }
    if (order == TRAVERSE_POSTORDER) {
        func(node, args);
    }
}

void traverseTree(BPTree* tree, TraverseOrder order, void(*func)(BPTNode*, va_list), ...) {
    va_list args;
    va_start(args, func);
    if (tree->root) {
        traverseTreeNode(tree->root, order, func, args);
    }
    va_end(args);
}

BPTree* createTree(void (*freeRecord)(void*), bool allowDuplicateKey) {
    BPTree* tree = malloc(sizeof(BPTree));
    *tree = (BPTree){
        .root = nullptr,
        .head = nullptr,
        .freeRecord = freeRecord,
        .allowDuplicateKey = allowDuplicateKey
    };
    return tree;
}

// 释放节点
// args 参数：void (*freeRecord)(void*)
void freeNode(BPTNode* node, va_list args) {
    void(*freeRecord)(void*) = va_arg(args, void*);
    if (node->isLeaf && freeRecord) {
        for (int i = 0; i < node->keyCount; i++) {
            freeRecord(node->records[i]);
        }
    }
    free(node);
}

void destroyTree(BPTree* tree) {
    traverseTree(tree, TRAVERSE_POSTORDER, freeNode, tree->freeRecord);
    free(tree);
}

// 找 node 位于 parent 中的下标
static inline int getNodeIndex(BPTNode* node) {
    int index = 0;
    BPTNode* parent = node->parent;
    if (!parent) {
        return -1;
    }
    while (index <= parent->keyCount && parent->children[index] != node) {
        index++;
    }
    return index;
}

typedef struct NodeFindResult {
    bool found; // 是否找到
    BPTNode* node; // 最接近节点，树为空时为 nullptr
    int i; // 找到时为对应下标，否则为应插入位置
} NodeFindResult;

NodeFindResult findNode(BPTree* tree, Key key) {
    BPTNode* p = tree->root;
    while (p) {
        // 里面用二分查找应该快些
        int i;
        if (p->isLeaf) {
            // 对于叶子节点
            for (i = 0; i < p->keyCount; i++) {
                if (key <= p->keys[i]) {
                    return (NodeFindResult) { key == p->keys[i], p, i };
                }
            }
            return (NodeFindResult) { false, p, i };
        } else {
            // 对于内部节点
            for (i = 0; i <= p->keyCount; i++) {
                if (i == p->keyCount || key < p->keys[i]) {
                    p = p->children[i];
                    break;
                }
            }
        }
    }
    // 除非树为空，否则不应出现这种情况
    return (NodeFindResult) { false, nullptr, -1 };
}

// 给老祖宗烧点纸报信 我家娃考上清华了
void updateAncestorKeys(BPTNode* node) {
    BPTNode* p = node;
    while (p->parent) {
        int nodeIndex = getNodeIndex(p);
        if (nodeIndex != 0) {
            p->parent->keys[nodeIndex - 1] = node->keys[0];
            printDebug("给祖宗烧了个 %llu", node->keys[0]);
            break;
        }
        p = p->parent;
    }
}

// 这里曾经有一段把我 CPU 干烧的函数
// 后来被我优化掉了 永远缅怀

// 在节点中插入关键字
// 写完这段感觉 CPU 和内存一起烧了
void nodeInsertKey(BPTNode* node, int index, Key key, void* pointer, int pointerDirection) {
    int moveCount = node->keyCount - index;
    memmove(node->keys + index + 1, node->keys + index, moveCount * sizeof(Key));
    memmove(node->pointers + index + 1 + pointerDirection, node->pointers + index + pointerDirection, (moveCount + (!node->isLeaf && !pointerDirection)) * sizeof(void*));
    node->keys[index] = key;
    node->pointers[index + pointerDirection] = pointer;
    node->keyCount++;
    if (!node->isLeaf && node->children[index + pointerDirection]) {
        node->children[index + pointerDirection]->parent = node;
    }
    if (node->isLeaf && index == 0) {
        updateAncestorKeys(node);
    }
}

// 在节点中删除关键字
// 写完这段感觉启用虚拟内存了
void nodeRemoveKey(BPTNode* node, int index, int pointerDirection) {
    int moveCount = node->keyCount - index - 1;
    memmove(node->keys + index, node->keys + index + 1, moveCount * sizeof(Key));
    memmove(node->pointers + index + pointerDirection, node->pointers + index + 1 + pointerDirection, (moveCount + (!node->isLeaf && !pointerDirection)) * sizeof(void*));
    node->keyCount--;
    if (node->isLeaf && index == 0 && node->keyCount) {
        updateAncestorKeys(node);
    }
}

// 从 index 起复制关键字
// 写完这段感觉机械飞升了（并没有）
void nodeMoveKeys(BPTNode* from, int fromIndex, BPTNode* to, int toIndex) {
    int moveCount = from->keyCount - fromIndex;
    memcpy(to->keys + toIndex, from->keys + fromIndex, moveCount * sizeof(Key));
    memcpy(to->pointers + toIndex, from->pointers + fromIndex, (moveCount + !from->isLeaf) * sizeof(void*));
    from->keyCount -= moveCount;
    to->keyCount += moveCount;
    if (!from->isLeaf) {
        for (int i = toIndex; i <= to->keyCount; i++) {
            to->children[i]->parent = to;
        }
    }
}

static inline Key getSmallestKey(BPTNode* node) {
    BPTNode* p = node;
    while (!p->isLeaf) {
        p = p->children[0];
    }
    return p->keys[0];
}

// 合并兄弟节点
// 写完这段感觉可以去睡觉了
void nodeMerge(BPTNode* left, BPTNode* right) {
    printDebug("尝试合并 [%s%llu, +%d] [%s%llu, +%d]", left->isLeaf ? "#" : "", left->keys[0], left->keyCount - 1, right->isLeaf ? "#" : "", right->keys[0], right->keyCount - 1);
    BPTNode* parent = left->parent;
    int nodeIndex = getNodeIndex(left);
    if (!left->isLeaf) {
        nodeInsertKey(left, left->keyCount, parent->keys[nodeIndex], nullptr, 1);
    } else {
        left->next = right->next;
    }
    nodeMoveKeys(right, 0, left, left->keyCount);
    nodeRemoveKey(parent, nodeIndex, 1);
    if (left->isLeaf) {
        updateAncestorKeys(left);
    }
    free(right);
    printDebug("合并结果 [%s%llu, +%d]", left->isLeaf ? "#" : "", left->keys[0], left->keyCount - 1);
}

void checkOverflow(BPTree* tree, BPTNode* node) {
    // 未溢出，爬了
    if (node->keyCount < B_PLUS_TREE_ORDER) {
        return;
    }

    // 拆之
    // 对于叶子节点，[0, ..., center - 1] 保留，[center, ..., keyCount - 1] 分裂，center 给爹
    // 对于内部节点，[0, ..., center - 1] 保留，[center + 1, ..., keyCount - 1] 分裂，center 给爹
    // ……啊呀，骇死我力！
    int center = node->keyCount / 2;
    BPTNode* split = malloc(sizeof(BPTNode)), * parent = node->parent;
    printDebug("节点溢出，将 %llu 插入爹", node->keys[center]);

    // 先确定分裂上去的爹
    if (parent) {
        // 沿用老爹
        nodeInsertKey(parent, getNodeIndex(node), node->keys[center], split, 1);
    } else {
        // 创建新爹
        parent = malloc(sizeof(BPTNode));
        *parent = (BPTNode){
            .isLeaf = false,
            .parent = nullptr,
            .keyCount = 1,
            .keys = { [0] = node->keys[center]},
            .children = { [0] = node,[1] = split}
        };

        node->parent = tree->root = parent;
    }

    // 再处理离婚的右半节点
    *split = (BPTNode){
        .isLeaf = node->isLeaf,
        .parent = parent
    };
    int splitStart = node->isLeaf ? center : center + 1;
    printDebug("将 %llu 至 %llu 分裂至新节点", node->keys[splitStart], node->keys[node->keyCount - 1]);
    nodeMoveKeys(node, splitStart, split, 0);

    // 家里的主心骨没了！
    if (!node->isLeaf) {
        node->keyCount--;
    }

    // 领取离婚证
    if (node->isLeaf) {
        split->next = node->next;
        node->next = split;
    }

    // 领完离婚证，再去看看爹
    checkOverflow(tree, parent);
}

// 插入记录
// 返回 false 表示对应 key 已存在
bool insertRecord(BPTree* tree, Key key, void* record) {
    NodeFindResult r = findNode(tree, key);
    BPTNode* node = r.node;
    if (!tree->allowDuplicateKey && r.found) {
        return false;
    }

    if (!node) {
        // 空树
        node = malloc(sizeof(BPTNode));
        *node = (BPTNode){
            .isLeaf = true,
            .parent = nullptr,
            .keyCount = 1,
            .keys = { [0] = key},
            .records = { [0] = record},
        };

        tree->head = tree->root = node;
    } else {
        // 非空树，总之，先插入了再说
        nodeInsertKey(node, r.i, key, record, 0);

        // 考虑溢出情况
        checkOverflow(tree, node);
    }
    return true;
}

void checkUnderflow(BPTree* tree, BPTNode* node) {
    // 我是老大 我才是老大
    if (!node->parent) {
        if (!node->keyCount) {
            // 感觉身体被掏空
            free(node);
            tree->head = tree->root = nullptr;
        }
        return;
    }

    if (node->keyCount >= (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        // 未下溢出，爬了
        return;
    }

    BPTNode* parent = node->parent;
    int nodeIndex = getNodeIndex(node);
    printDebug("节点 [%s%llu, +%d] 下溢", node->isLeaf ? "#" : "", node->keys[0], node->keyCount - 1);

    // 兄弟兄弟，在家吗？
    BPTNode* rightSibling = nodeIndex < parent->keyCount ? parent->children[nodeIndex + 1] : nullptr;
    if (rightSibling && rightSibling->keyCount > (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        // 兄弟兄弟，借我点钱
        printDebug("向右兄弟借了 %llu", rightSibling->keys[0]);
        nodeInsertKey(node, node->keyCount, node->isLeaf ? rightSibling->keys[0] : getSmallestKey(rightSibling), rightSibling->pointers[0], !rightSibling->isLeaf);
        nodeRemoveKey(rightSibling, 0, 0);
        parent->keys[nodeIndex] = getSmallestKey(rightSibling);
        return;
    }

    // 兄弟兄弟，在家吗？
    BPTNode* leftSibling = nodeIndex > 0 ? parent->children[nodeIndex - 1] : nullptr;
    if (leftSibling && leftSibling->keyCount > (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        // 兄弟兄弟，借我点钱
        printDebug("向左兄弟借了 %llu", leftSibling->keys[leftSibling->keyCount - 1]);
        nodeInsertKey(node, 0, node->isLeaf ? leftSibling->keys[leftSibling->keyCount - 1] : getSmallestKey(node->children[0]), leftSibling->pointers[leftSibling->keyCount - leftSibling->isLeaf], 0);
        nodeRemoveKey(leftSibling, leftSibling->keyCount - 1, !leftSibling->isLeaf);
        parent->keys[nodeIndex - 1] = getSmallestKey(node);
        return;
    }

    // 兄弟穷得叮当响
    BPTNode* merger, * merged;
    if (rightSibling) {
        // 兄弟，你好香
        printDebug("把右兄弟吃掉");
        merger = node;
        merged = rightSibling;
    } else if (leftSibling) {
        // “兄弟，你好香”
        printDebug("被左兄弟吃掉");
        merger = leftSibling;
        merged = node;
    } else {
        // 理论上不会
        return;
    }
    nodeMerge(merger, merged);

    // 和兄弟贴贴了，看看爹怎么说
    if (tree->root == parent && !parent->keyCount) {
        // 爹：我没意见
        printDebug("爹死了，我才是新爹！");
        free(parent);
        tree->root = merger;
        merger->parent = nullptr;
    } else {
        // 爹听完坍缩了
        checkUnderflow(tree, parent);
    }
}

// 对于 allowDuplicateKey 的 B+ 树，才需要提供 record
// 返回被删除的 record 指针，需要注意其内存可能已经被释放
void* removeRecord(BPTree* tree, Key key, void* record) {
    NodeFindResult r = findNode(tree, key);
    BPTNode* node = r.node;
    int i = r.i;
    if (!r.found) {
        return nullptr;
    }

    if (tree->allowDuplicateKey) {
        while (node && node->keys[i] == key && node->records[i] != record) {
            if (i < node->keyCount - 1) {
                i++;
            } else {
                node = node->next;
                i = 0;
            }
        }
        if (!node || node->keys[i] != key || node->records[i] != record) {
            return nullptr;
        }
    }

    // 先删了再说
    record = node->records[i];
    if (tree->freeRecord) {
        tree->freeRecord(record);
    }
    nodeRemoveKey(node, i, 0);

    // 考虑下溢出情况
    checkUnderflow(tree, node);
    return record;
}

bool replaceRecord(BPTree* tree, Key key, void* record) {
    NodeFindResult r = findNode(tree, key);
    if (!r.found) {
        return false;
    }

    void* oldRecord = r.node->records[r.i];
    if (record == oldRecord) {
        return false;
    }
    r.node->records[r.i] = record;
    if (tree->freeRecord) {
        tree->freeRecord(oldRecord);
    }
    return true;
}

void* findRecord(BPTree* tree, Key key) {
    NodeFindResult r = findNode(tree, key);
    return r.found ? r.node->records[r.i] : nullptr;
}

void findRecordRange(BPTree* tree, Key min, Key max, void (*operation)(void*)) {
    NodeFindResult r = findNode(tree, min);

    int i = r.i;
    BPTNode* p = r.node;

    while (p) {
        for (i = p == r.node ? i : 0; i < p->keyCount; i++) {
            if (p->keys[i] < min) {
                continue;
            } else if (p->keys[i] > max) {
                return;
            }
            operation(p->records[i]);
        }
        p = p->next;
    }
}

// 将节点信息写入 mermaid
// args 参数：FILE* fp
void putsNodeMermaid(BPTNode* node, va_list args) {
    FILE* fp = va_arg(args, FILE*);

    // 节点定义
    fprintf(fp, "\t%p[", node);
    for (int i = 0; i < node->keyCount; i++) {
        if (node->isLeaf) {
            fprintf(fp, "#");
        }
        fprintf(fp, "%llu%s", node->keys[i], i != node->keyCount - 1 ? ", " : "]\n");
    }
    // 这种情况不应该出现，但是以防万一
    if (!node->keyCount) {
        fprintf(fp, "空节点]\n");
    }

    // 与父节点箭头
    // 加上以后会让图看起来比较乱
    // if (node->parent) {
    //     fprintf(fp, "\t%p --> %p\n", node, node->parent);
    // }

    if (!node->isLeaf) {
        // 与子节点箭头
        for (int i = 0; i <= node->keyCount; i++) {
            fprintf(fp, "\t%p -- %d --> %p\n", node, i, node->children[i]);
        }
    } else {
        // 与链表下一节点箭头
        // 忽略该箭头，以将所有叶子节点布置在同一层
        // 否则由于 mermaid 顺序（从上到下），图像会变得非常扭曲丑陋
        // if (node->isLeaf && node->next) {
        //     fprintf(fp, "\t%p --> %p\n", node, node->next);
        // }
    }
}

void saveTreeMermaid(BPTree* tree, char* filename, bool openAfterSave) {
    FILE* fp = fopen(filename, "w");

    // 笑死 我直接硬编码 HTML
    fputs("<!DOCTYPE html><html><head><script src=\"https://unpkg.com/mermaid/dist/mermaid.min.js\"></script>", fp);
    fputs("<script>mermaid.initialize({startOnLoad:true});</script><title>B+Tree</title></head><body><div class=\"mermaid\">\n", fp);
    fputs("---\nconfig:\n  theme: 'neutral'\n  look: handDrawn\n  themeCSS: '.cluster-label { display: none; }'\n---\ngraph TD\n", fp);
    traverseTree(tree, TRAVERSE_PREORDER, putsNodeMermaid, fp);
    fputs("\tsubgraph leaves\n", fp);
    BPTNode* p = tree->head;
    while (p) {
        fprintf(fp, "\t\t%p\n", p);
        p = p->next;
    }
    fputs("\tend\n</div></body></html>", fp);
    fclose(fp);

    if (openAfterSave) {
        system(filename);
    }
}

// 检查节点是否合法，调试用
// args 参数：BPTree* tree, bool recordIsKey
void checkNodeLegitimacy(BPTNode* node, va_list args) {
    BPTree* tree = va_arg(args, BPTree*);
    bool recordIsKey = (bool)va_arg(args, int); // bool 经过 va_arg 被提升为 int

    // 判断键数
    if (node->keyCount >= B_PLUS_TREE_ORDER) {
        printFatal("节点 %p 错误：键数过多", node);
    }
    if (tree->root != node && node->keyCount < (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        printFatal("节点 %p 错误：键数过少", node);
    }

    // 判断键关系
    Key lastKey = node->keys[0];
    for (int i = 1; i < node->keyCount; i++) {
        if (lastKey > node->keys[i]) {
            printFatal("节点 %p 错误：键 [%d %d] 顺序异常", node, lastKey, node->keys[i]);
        } else if (lastKey == node->keys[i] && !tree->allowDuplicateKey) {
            printFatal("节点 %p 错误：重复键 [%d]", lastKey);
        }
        lastKey = node->keys[i];
    }

    if (node->isLeaf) {
        // 判断链表连接关系
        if (node->next) {
            if (lastKey > node->next->keys[0]) {
                printFatal("节点 %p 错误：链表键顺序异常", node, node->next);
            } else if (lastKey == node->next->keys[0] && !tree->allowDuplicateKey) {
                printFatal("节点 %p 错误：链表重复键");
            }
        } else {
            BPTNode* p = tree->root;
            while (!p->isLeaf) {
                p = p->children[p->keyCount];
            }
            if (node != p) {
                printFatal("错误：链表尾非末个节点");
            }
        }

        // 判断记录
        if (recordIsKey) {
            for (int i = 0; i < node->keyCount; i++) {
                if (node->keys[i] != *(Key*)node->records[i]) {
                    printFatal("节点 %p 错误：[%llu] 键值不等 (%llu)", node->keys[i], *(Key*)node->records[i]);
                }
            }
        }
    } else {
        // 判断亲子关系
        for (int i = 0; i < node->keyCount; i++) {
            Key currentKey = node->keys[i];
            BPTNode* leftChild = node->children[i], * rightChild = node->children[i + 1], * p;
            if (leftChild->keys[leftChild->keyCount - 1] > currentKey) {
                printFatal("节点 %p 错误：[%d] 左儿子键过大", node, currentKey, leftChild);
            }
            if (rightChild->keys[0] < currentKey) {
                printFatal("节点 %p 错误：[%d] 右儿子键过小", node, currentKey, rightChild);
            }
            p = rightChild;
            while (!p->isLeaf) {
                p = p->children[0];
            }
            if (p->keys[0] != currentKey) {
                printFatal("节点 %p 错误：[%d] 键值不在直接后继上", node, currentKey);
            }
        }
        for (int i = 0; i <= node->keyCount; i++) {
            if (node->children[i]->parent != node) {
                printFatal("节点 %p 错误：第 %d 个儿子的爹不是自己", node, i);
            }
        }
    }
}

// 检查树是否合法，调试用
void checkTreeLegitimacy(BPTree* tree, bool recordIsKey) {
    BPTNode* p = tree->root;
    if (!p) {
        return;
    }
    while (!p->isLeaf) {
        p = p->children[0];
    }
    if (p != tree->head) {
        printFatal("错误：链表头非首个节点");
    }
    traverseTree(tree, TRAVERSE_PREORDER, checkNodeLegitimacy, tree, recordIsKey);
}