/* Expected output:
1 3
*/
#include <stdio.h>

static int first(int *p) {
    return p[0];
}

int main(void) {
    int a[3] = {1, 2, 3};
    printf("%d %d\n", first(a), (int)(sizeof(a) / sizeof(a[0])));
    return 0;
}
