/* Expected output:
7 8 9
*/
#include <stdio.h>

static int choose(int a, int b, int c) {
    return a ? 7 : b ? 8 : c ? 9 : 10;
}

int main(void) {
    printf("%d %d %d\n", choose(1, 0, 0), choose(0, 1, 0), choose(0, 0, 1));
    return 0;
}
