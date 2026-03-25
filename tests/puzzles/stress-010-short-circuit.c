/* Expected output:
0 1 1
*/
#include <stdio.h>

int main(void) {
    int x = 0;
    int y = 0;

    if (x && (y = 1)) {
        y = 99;
    }
    if (!x || (y = 2)) {
        y += 1;
    }
    printf("%d %d %d\n", x, y == 1, y);
    return 0;
}
