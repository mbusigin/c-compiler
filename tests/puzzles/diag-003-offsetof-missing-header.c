/* Expected diagnostic unless offsetof is treated as a builtin without header. */
#include <stdio.h>

struct S {
    char c;
    int i;
};

int main(void) {
    struct S s = {'A', 7};
    printf("%zu\n", offsetof(struct S, i));
    return 0;
}
