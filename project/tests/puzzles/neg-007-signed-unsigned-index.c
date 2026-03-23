/* Expected output:
0 1
*/
#include <stdio.h>

static int ok_index(int n, int i) {
    return i >= 0 && (unsigned)i < (unsigned)n;
}

int main(void) {
    printf("%d %d\n", ok_index(4, -1), ok_index(4, 3));
    return 0;
}
