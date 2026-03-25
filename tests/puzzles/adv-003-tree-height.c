/* Expected output:
4
*/
#include <stdio.h>

struct Node {
    int key;
    struct Node *left;
    struct Node *right;
};

static int max2(int a, int b) {
    return a > b ? a : b;
}

static int height(const struct Node *node) {
    if (node == 0) {
        return 0;
    }
    return 1 + max2(height(node->left), height(node->right));
}

int main(void) {
    struct Node n2 = {2, 0, 0};
    struct Node n4 = {4, 0, 0};
    struct Node n3 = {3, &n2, &n4};
    struct Node n7 = {7, 0, 0};
    struct Node n6 = {6, 0, &n7};
    struct Node n5 = {5, &n3, &n6};
    struct Node n9 = {9, 0, 0};
    struct Node n8 = {8, &n5, &n9};

    printf("%d\n", height(&n8));
    return 0;
}
