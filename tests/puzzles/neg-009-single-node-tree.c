/* Expected output:
1 99
*/
#include <stdio.h>

struct Node {
    int key;
    struct Node *left;
    struct Node *right;
};

static int height(const struct Node *n) {
    if (n == 0) {
        return 0;
    }
    return 1;
}

int main(void) {
    struct Node n = {99, 0, 0};
    printf("%d %d\n", height(&n), n.key);
    return 0;
}
