/* Expected output:
42 0
*/
#include <stdio.h>

static int max1(const int *a, int n) {
    int i;
    int best = a[0];

    for (i = 1; i < n; i++) {
        if (a[i] > best) {
            best = a[i];
        }
    }
    return best;
}

int main(void) {
    int a[1] = {42};
    printf("%d %d\n", max1(a, 1), max1(a, 1) - 42);
    return 0;
}
