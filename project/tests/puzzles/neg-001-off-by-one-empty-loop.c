/* Expected output:
0
*/
#include <stdio.h>

static int sum_n(const int *a, int n) {
    int i;
    int sum = 0;

    for (i = 0; i < n; i++) {
        sum += a[i];
    }
    return sum;
}

int main(void) {
    int a[1] = {99};
    printf("%d\n", sum_n(a, 0));
    return 0;
}
