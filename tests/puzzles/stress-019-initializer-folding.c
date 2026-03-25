/* Expected output:
14 9
*/
#include <stdio.h>

static int a[] = {1 + 2, 3 * 4 - 1};
static int b[2] = {[0] = 9, [1] = 4 + 1};

int main(void) {
    printf("%d %d\n", a[0] + a[1], b[0]);
    return 0;
}
