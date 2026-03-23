/* Expected output:
0 1
*/
#include <stdio.h>

int main(void) {
    int a = -1;
    unsigned b = 1;

    printf("%d %d\n", a < b, a > b);
    return 0;
}
