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

// �ͷŽڵ�
// args ������void (*freeRecord)(void*)
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

// �� node λ�� parent �е��±�
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
    bool found; // �Ƿ��ҵ�
    BPTNode* node; // ��ӽ��ڵ㣬��Ϊ��ʱΪ nullptr
    int i; // �ҵ�ʱΪ��Ӧ�±꣬����ΪӦ����λ��
} NodeFindResult;

NodeFindResult findNode(BPTree* tree, Key key) {
    BPTNode* p = tree->root;
    while (p) {
        // �����ö��ֲ���Ӧ�ÿ�Щ
        int i;
        if (p->isLeaf) {
            // ����Ҷ�ӽڵ�
            for (i = 0; i < p->keyCount; i++) {
                if (key <= p->keys[i]) {
                    return (NodeFindResult) { key == p->keys[i], p, i };
                }
            }
            return (NodeFindResult) { false, p, i };
        } else {
            // �����ڲ��ڵ�
            for (i = 0; i <= p->keyCount; i++) {
                if (i == p->keyCount || key < p->keys[i]) {
                    p = p->children[i];
                    break;
                }
            }
        }
    }
    // ������Ϊ�գ�����Ӧ�����������
    return (NodeFindResult) { false, nullptr, -1 };
}

// ���������յ�ֽ���� �Ҽ��޿����廪��
void updateAncestorKeys(BPTNode* node) {
    BPTNode* p = node;
    while (p->parent) {
        int nodeIndex = getNodeIndex(p);
        if (nodeIndex != 0) {
            p->parent->keys[nodeIndex - 1] = node->keys[0];
            printDebug("���������˸� %llu", node->keys[0]);
            break;
        }
        p = p->parent;
    }
}

// ����������һ�ΰ��� CPU ���յĺ���
// ���������Ż����� ��Զ�廳

// �ڽڵ��в���ؼ���
// д����θо� CPU ���ڴ�һ������
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

// �ڽڵ���ɾ���ؼ���
// д����θо����������ڴ���
void nodeRemoveKey(BPTNode* node, int index, int pointerDirection) {
    int moveCount = node->keyCount - index - 1;
    memmove(node->keys + index, node->keys + index + 1, moveCount * sizeof(Key));
    memmove(node->pointers + index + pointerDirection, node->pointers + index + 1 + pointerDirection, (moveCount + (!node->isLeaf && !pointerDirection)) * sizeof(void*));
    node->keyCount--;
    if (node->isLeaf && index == 0 && node->keyCount) {
        updateAncestorKeys(node);
    }
}

// �� index ���ƹؼ���
// д����θо���е�����ˣ���û�У�
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

// �ϲ��ֵܽڵ�
// д����θо�����ȥ˯����
void nodeMerge(BPTNode* left, BPTNode* right) {
    printDebug("���Ժϲ� [%s%llu, +%d] [%s%llu, +%d]", left->isLeaf ? "#" : "", left->keys[0], left->keyCount - 1, right->isLeaf ? "#" : "", right->keys[0], right->keyCount - 1);
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
    printDebug("�ϲ���� [%s%llu, +%d]", left->isLeaf ? "#" : "", left->keys[0], left->keyCount - 1);
}

void checkOverflow(BPTree* tree, BPTNode* node) {
    // δ���������
    if (node->keyCount < B_PLUS_TREE_ORDER) {
        return;
    }

    // ��֮
    // ����Ҷ�ӽڵ㣬[0, ..., center - 1] ������[center, ..., keyCount - 1] ���ѣ�center ����
    // �����ڲ��ڵ㣬[0, ..., center - 1] ������[center + 1, ..., keyCount - 1] ���ѣ�center ����
    // ������ѽ������������
    int center = node->keyCount / 2;
    BPTNode* split = malloc(sizeof(BPTNode)), * parent = node->parent;
    printDebug("�ڵ�������� %llu �����", node->keys[center]);

    // ��ȷ��������ȥ�ĵ�
    if (parent) {
        // �����ϵ�
        nodeInsertKey(parent, getNodeIndex(node), node->keys[center], split, 1);
    } else {
        // �����µ�
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

    // �ٴ��������Ұ�ڵ�
    *split = (BPTNode){
        .isLeaf = node->isLeaf,
        .parent = parent
    };
    int splitStart = node->isLeaf ? center : center + 1;
    printDebug("�� %llu �� %llu �������½ڵ�", node->keys[splitStart], node->keys[node->keyCount - 1]);
    nodeMoveKeys(node, splitStart, split, 0);

    // ��������Ĺ�û�ˣ�
    if (!node->isLeaf) {
        node->keyCount--;
    }

    // ��ȡ���֤
    if (node->isLeaf) {
        split->next = node->next;
        node->next = split;
    }

    // �������֤����ȥ������
    checkOverflow(tree, parent);
}

// �����¼
// ���� false ��ʾ��Ӧ key �Ѵ���
bool insertRecord(BPTree* tree, Key key, void* record) {
    NodeFindResult r = findNode(tree, key);
    BPTNode* node = r.node;
    if (!tree->allowDuplicateKey && r.found) {
        return false;
    }

    if (!node) {
        // ����
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
        // �ǿ�������֮���Ȳ�������˵
        nodeInsertKey(node, r.i, key, record, 0);

        // ����������
        checkOverflow(tree, node);
    }
    return true;
}

void checkUnderflow(BPTree* tree, BPTNode* node) {
    // �����ϴ� �Ҳ����ϴ�
    if (!node->parent) {
        if (!node->keyCount) {
            // �о����屻�Ϳ�
            free(node);
            tree->head = tree->root = nullptr;
        }
        return;
    }

    if (node->keyCount >= (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        // δ�����������
        return;
    }

    BPTNode* parent = node->parent;
    int nodeIndex = getNodeIndex(node);
    printDebug("�ڵ� [%s%llu, +%d] ����", node->isLeaf ? "#" : "", node->keys[0], node->keyCount - 1);

    // �ֵ��ֵܣ��ڼ���
    BPTNode* rightSibling = nodeIndex < parent->keyCount ? parent->children[nodeIndex + 1] : nullptr;
    if (rightSibling && rightSibling->keyCount > (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        // �ֵ��ֵܣ����ҵ�Ǯ
        printDebug("�����ֵܽ��� %llu", rightSibling->keys[0]);
        nodeInsertKey(node, node->keyCount, node->isLeaf ? rightSibling->keys[0] : getSmallestKey(rightSibling), rightSibling->pointers[0], !rightSibling->isLeaf);
        nodeRemoveKey(rightSibling, 0, 0);
        parent->keys[nodeIndex] = getSmallestKey(rightSibling);
        return;
    }

    // �ֵ��ֵܣ��ڼ���
    BPTNode* leftSibling = nodeIndex > 0 ? parent->children[nodeIndex - 1] : nullptr;
    if (leftSibling && leftSibling->keyCount > (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        // �ֵ��ֵܣ����ҵ�Ǯ
        printDebug("�����ֵܽ��� %llu", leftSibling->keys[leftSibling->keyCount - 1]);
        nodeInsertKey(node, 0, node->isLeaf ? leftSibling->keys[leftSibling->keyCount - 1] : getSmallestKey(node->children[0]), leftSibling->pointers[leftSibling->keyCount - leftSibling->isLeaf], 0);
        nodeRemoveKey(leftSibling, leftSibling->keyCount - 1, !leftSibling->isLeaf);
        parent->keys[nodeIndex - 1] = getSmallestKey(node);
        return;
    }

    // �ֵ���ö�����
    BPTNode* merger, * merged;
    if (rightSibling) {
        // �ֵܣ������
        printDebug("�����ֵܳԵ�");
        merger = node;
        merged = rightSibling;
    } else if (leftSibling) {
        // ���ֵܣ�����㡱
        printDebug("�����ֵܳԵ�");
        merger = leftSibling;
        merged = node;
    } else {
        // �����ϲ���
        return;
    }
    nodeMerge(merger, merged);

    // ���ֵ������ˣ���������ô˵
    if (tree->root == parent && !parent->keyCount) {
        // ������û���
        printDebug("�����ˣ��Ҳ����µ���");
        free(parent);
        tree->root = merger;
        merger->parent = nullptr;
    } else {
        // ������̮����
        checkUnderflow(tree, parent);
    }
}

// ���� allowDuplicateKey �� B+ ��������Ҫ�ṩ record
// ���ر�ɾ���� record ָ�룬��Ҫע�����ڴ�����Ѿ����ͷ�
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

    // ��ɾ����˵
    record = node->records[i];
    if (tree->freeRecord) {
        tree->freeRecord(record);
    }
    nodeRemoveKey(node, i, 0);

    // ������������
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

// ���ڵ���Ϣд�� mermaid
// args ������FILE* fp
void putsNodeMermaid(BPTNode* node, va_list args) {
    FILE* fp = va_arg(args, FILE*);

    // �ڵ㶨��
    fprintf(fp, "\t%p[", node);
    for (int i = 0; i < node->keyCount; i++) {
        if (node->isLeaf) {
            fprintf(fp, "#");
        }
        fprintf(fp, "%llu%s", node->keys[i], i != node->keyCount - 1 ? ", " : "]\n");
    }
    // ���������Ӧ�ó��֣������Է���һ
    if (!node->keyCount) {
        fprintf(fp, "�սڵ�]\n");
    }

    // �븸�ڵ��ͷ
    // �����Ժ����ͼ�������Ƚ���
    // if (node->parent) {
    //     fprintf(fp, "\t%p --> %p\n", node, node->parent);
    // }

    if (!node->isLeaf) {
        // ���ӽڵ��ͷ
        for (int i = 0; i <= node->keyCount; i++) {
            fprintf(fp, "\t%p -- %d --> %p\n", node, i, node->children[i]);
        }
    } else {
        // ��������һ�ڵ��ͷ
        // ���Ըü�ͷ���Խ�����Ҷ�ӽڵ㲼����ͬһ��
        // �������� mermaid ˳�򣨴��ϵ��£���ͼ����÷ǳ�Ť����ª
        // if (node->isLeaf && node->next) {
        //     fprintf(fp, "\t%p --> %p\n", node, node->next);
        // }
    }
}

void saveTreeMermaid(BPTree* tree, char* filename, bool openAfterSave) {
    FILE* fp = fopen(filename, "w");

    // Ц�� ��ֱ��Ӳ���� HTML
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

// ���ڵ��Ƿ�Ϸ���������
// args ������BPTree* tree, bool recordIsKey
void checkNodeLegitimacy(BPTNode* node, va_list args) {
    BPTree* tree = va_arg(args, BPTree*);
    bool recordIsKey = (bool)va_arg(args, int); // bool ���� va_arg ������Ϊ int

    // �жϼ���
    if (node->keyCount >= B_PLUS_TREE_ORDER) {
        printFatal("�ڵ� %p ���󣺼�������", node);
    }
    if (tree->root != node && node->keyCount < (B_PLUS_TREE_ORDER + 1) / 2 - 1) {
        printFatal("�ڵ� %p ���󣺼�������", node);
    }

    // �жϼ���ϵ
    Key lastKey = node->keys[0];
    for (int i = 1; i < node->keyCount; i++) {
        if (lastKey > node->keys[i]) {
            printFatal("�ڵ� %p ���󣺼� [%d %d] ˳���쳣", node, lastKey, node->keys[i]);
        } else if (lastKey == node->keys[i] && !tree->allowDuplicateKey) {
            printFatal("�ڵ� %p �����ظ��� [%d]", lastKey);
        }
        lastKey = node->keys[i];
    }

    if (node->isLeaf) {
        // �ж��������ӹ�ϵ
        if (node->next) {
            if (lastKey > node->next->keys[0]) {
                printFatal("�ڵ� %p ���������˳���쳣", node, node->next);
            } else if (lastKey == node->next->keys[0] && !tree->allowDuplicateKey) {
                printFatal("�ڵ� %p ���������ظ���");
            }
        } else {
            BPTNode* p = tree->root;
            while (!p->isLeaf) {
                p = p->children[p->keyCount];
            }
            if (node != p) {
                printFatal("��������β��ĩ���ڵ�");
            }
        }

        // �жϼ�¼
        if (recordIsKey) {
            for (int i = 0; i < node->keyCount; i++) {
                if (node->keys[i] != *(Key*)node->records[i]) {
                    printFatal("�ڵ� %p ����[%llu] ��ֵ���� (%llu)", node->keys[i], *(Key*)node->records[i]);
                }
            }
        }
    } else {
        // �ж����ӹ�ϵ
        for (int i = 0; i < node->keyCount; i++) {
            Key currentKey = node->keys[i];
            BPTNode* leftChild = node->children[i], * rightChild = node->children[i + 1], * p;
            if (leftChild->keys[leftChild->keyCount - 1] > currentKey) {
                printFatal("�ڵ� %p ����[%d] ����Ӽ�����", node, currentKey, leftChild);
            }
            if (rightChild->keys[0] < currentKey) {
                printFatal("�ڵ� %p ����[%d] �Ҷ��Ӽ���С", node, currentKey, rightChild);
            }
            p = rightChild;
            while (!p->isLeaf) {
                p = p->children[0];
            }
            if (p->keys[0] != currentKey) {
                printFatal("�ڵ� %p ����[%d] ��ֵ����ֱ�Ӻ����", node, currentKey);
            }
        }
        for (int i = 0; i <= node->keyCount; i++) {
            if (node->children[i]->parent != node) {
                printFatal("�ڵ� %p ���󣺵� %d �����ӵĵ������Լ�", node, i);
            }
        }
    }
}

// ������Ƿ�Ϸ���������
void checkTreeLegitimacy(BPTree* tree, bool recordIsKey) {
    BPTNode* p = tree->root;
    if (!p) {
        return;
    }
    while (!p->isLeaf) {
        p = p->children[0];
    }
    if (p != tree->head) {
        printFatal("��������ͷ���׸��ڵ�");
    }
    traverseTree(tree, TRAVERSE_PREORDER, checkNodeLegitimacy, tree, recordIsKey);
}