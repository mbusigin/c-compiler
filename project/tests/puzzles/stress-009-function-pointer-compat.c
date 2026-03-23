/* Should compile successfully and print:
12
*/
#include <stdio.h>

static int apply(int (*fn)(int), int x) {
    return fn(x);
}

static int add2(int x) {
    return x + 2;
}

int main(void) {
    printf("%d\n", apply(add2, 10));
    return 0;
}
