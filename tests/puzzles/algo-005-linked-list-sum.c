/* Expected output:
14
*/
#include <stdio.h>

struct Node {
    int value;
    struct Node *next;
};

static int sum_list(const struct Node *node) {
    int sum = 0;

    while (node != 0) {
        sum += node->value;
        node = node->next;
    }
    return sum;
}

int main(void) {
    struct Node c = {5, 0};
    struct Node b = {7, &c};
    struct Node a = {2, &b};

    printf("%d\n", sum_list(&a));
    return 0;
}
