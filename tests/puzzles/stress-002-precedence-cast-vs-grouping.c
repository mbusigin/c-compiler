/* Expected output:
13 24
*/
#include <stdio.h>

int main(void) {
    int x = 3;
    int y = 5;

    printf("%d %d\n", (int)(x + y * 2), ((int)(x + y)) * 3);
    return 0;
}
