/* Expected output:
10
*/
#include <stdio.h>

#define SQR(x) x * x

int main(void) {
    printf("%d\n", 2 * SQR(3 + 1));
    return 0;
}
