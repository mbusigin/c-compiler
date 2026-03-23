/* Expected output:
21
*/
#include <stdio.h>

static int gcd(int a, int b) {
    while (b != 0) {
        int r = a % b;
        a = b;
        b = r;
    }
    return a;
}

int main(void) {
    printf("%d\n", gcd(1071, 462));
    return 0;
}
