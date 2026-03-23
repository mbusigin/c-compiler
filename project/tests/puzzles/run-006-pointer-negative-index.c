/* Expected output:
2
*/
#include <stdio.h>

int main(void) {
    int a[] = {0, 1, 2, 3, 4};
    int *p = a + 4;
    printf("%d\n", p[-2]);
    return 0;
}
