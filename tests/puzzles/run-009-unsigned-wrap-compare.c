/* Expected output on targets with 32-bit unsigned:
4294967295 0
*/
#include <stdio.h>

int main(void) {
    unsigned u = 0;
    printf("%u %d\n", u - 1, (u - 1) < 0);
    return 0;
}
