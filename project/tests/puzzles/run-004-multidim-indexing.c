/* Expected output:
6 6
*/
#include <stdio.h>

int main(void) {
    int a[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };
    int (*p)[3] = a;
    printf("%d %d\n", p[1][2], *(*(a + 1) + 2));
    return 0;
}
