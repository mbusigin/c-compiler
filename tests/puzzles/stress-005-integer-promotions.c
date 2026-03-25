/* Expected output:
1 1
*/
#include <stdio.h>

int main(void) {
    unsigned char a = 250;
    unsigned char b = 10;
    int sum = a + b;

    printf("%d %d\n", sum == 260, (unsigned char)(a + b) == 4);
    return 0;
}
