/* Expected output:
6
*/
#include <stdio.h>

static int popcount(unsigned x) {
    int count = 0;

    while (x != 0) {
        count += (int)(x & 1u);
        x >>= 1;
    }
    return count;
}

int main(void) {
    printf("%d\n", popcount(173u));
    return 0;
}
