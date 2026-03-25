/* Expected output:
8 3 10 1 6 14 4 7 13
*/
#include <stdio.h>

struct Node {
    int key;
    struct Node *left;
    struct Node *right;
};

static void level_order(struct Node *root) {
    struct Node *q[16];
    int head = 0;
    int tail = 0;
    int first = 1;

    q[tail++] = root;
    while (head < tail) {
        struct Node *node = q[head++];
        printf("%s%d", first ? "" : " ", node->key);
        first = 0;
        if (node->left != 0) {
            q[tail++] = node->left;
        }
        if (node->right != 0) {
            q[tail++] = node->right;
        }
    }
    printf("\n");
}

int main(void) {
    struct Node n1 = {1, 0, 0};
    struct Node n4 = {4, 0, 0};
    struct Node n7 = {7, 0, 0};
    struct Node n13 = {13, 0, 0};
    struct Node n6 = {6, &n4, &n7};
    struct Node n14 = {14, &n13, 0};
    struct Node n3 = {3, &n1, &n6};
    struct Node n10 = {10, 0, &n14};
    struct Node n8 = {8, &n3, &n10};

    level_order(&n8);
    return 0;
}
