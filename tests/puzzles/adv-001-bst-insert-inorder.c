/* Expected output:
1 3 4 6 7 8 10 13 14
*/
#include <stdio.h>

struct Node {
    int key;
    struct Node *left;
    struct Node *right;
};

static void insert(struct Node **root, struct Node *node) {
    while (*root != 0) {
        if (node->key < (*root)->key) {
            root = &(*root)->left;
        } else {
            root = &(*root)->right;
        }
    }
    *root = node;
}

static void inorder(const struct Node *node, int *first) {
    if (node == 0) {
        return;
    }
    inorder(node->left, first);
    printf("%s%d", *first ? "" : " ", node->key);
    *first = 0;
    inorder(node->right, first);
}

int main(void) {
    int i;
    int first = 1;
    int keys[9] = {8, 3, 10, 1, 6, 14, 4, 7, 13};
    struct Node nodes[9];
    struct Node *root = 0;

    for (i = 0; i < 9; i++) {
        nodes[i].key = keys[i];
        nodes[i].left = 0;
        nodes[i].right = 0;
        insert(&root, &nodes[i]);
    }
    inorder(root, &first);
    printf("\n");
    return 0;
}
