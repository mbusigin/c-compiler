/* Expected output:
7 9
*/
#include <stdio.h>

int main(void) {
    int a[3] = {7, 8, 9};
    int *p = a;

    *p = a[0];
    *(p + 2) = 9;
    printf("%d %d\n", a[0], p[2]);
    return 0;
}
