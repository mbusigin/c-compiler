/* Expected output on 64-bit targets with 4-byte int:
8 40
*/
#include <stdio.h>

int f(int a[10]) {
    return (int)sizeof(a);
}

int main(void) {
    int x[10];
    printf("%d %d\n", f(x), (int)sizeof(x));
    return 0;
}
