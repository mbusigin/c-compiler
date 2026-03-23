/* Expected output:
2
*/
#include <stdio.h>

int main(void) {
    int a[3] = {1, 2, 3};
    void *vp = a;
    int *p = (int *)vp;

    printf("%d\n", p[1]);
    return 0;
}
