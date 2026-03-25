/* Expected output on 64-bit targets:
3 8
*/
#include <stdio.h>

int main(void) {
    int a[3] = {1, 2, 3};
    printf("%d %d\n",
           (int)(sizeof(a) / sizeof(a[0])),
           (int)sizeof(a + 0));
    return 0;
}
