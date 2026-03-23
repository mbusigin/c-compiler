/* Expected output:
1 0
*/
#include <stdio.h>

struct Node {
    int key;
    struct Node *left;
    struct Node *right;
};

static int contains(const struct Node *root, int key) {
    while (root != 0) {
        if (key == root->key) {
            return 1;
        }
        if (key < root->key) {
            root = root->left;
        } else {
            root = root->right;
        }
    }
    return 0;
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

    printf("%d %d\n", contains(&n8, 7), contains(&n8, 2));
    return 0;
}
