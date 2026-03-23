/* Expected output:
40
*/
#include <stdio.h>

static int map(int x) {
    switch (x) {
        case 0: return 10;
        case 1: return 20;
        case 2:
        case 3: return 30;
        default: return 40;
    }
}

int main(void) {
    printf("%d\n", map(8));
    return 0;
}
