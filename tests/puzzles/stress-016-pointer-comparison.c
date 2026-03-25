/* Expected output:
1 1
*/
#include <stdio.h>

int main(void) {
    int a[4] = {0, 1, 2, 3};
    int *p = &a[1];
    int *q = &a[3];

    printf("%d %d\n", p < q, q > p);
    return 0;
}
