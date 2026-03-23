/* Should compile successfully and print:
1
*/
#include <stddef.h>
#include <stdio.h>

struct S {
    char c;
    int i;
};

int main(void) {
    printf("%d\n", offsetof(struct S, i) >= 1);
    return 0;
}
