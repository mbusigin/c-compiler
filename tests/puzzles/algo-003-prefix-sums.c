/* Expected output:
3 4 8 9 14
*/
#include <stdio.h>

static void prefix_sums(int *a, int n) {
    int i;

    for (i = 1; i < n; i++) {
        a[i] += a[i - 1];
    }
}

int main(void) {
    int i;
    int a[5] = {3, 1, 4, 1, 5};

    prefix_sums(a, 5);
    for (i = 0; i < 5; i++) {
        printf("%d%c", a[i], i == 4 ? '\n' : ' ');
    }
    return 0;
}
