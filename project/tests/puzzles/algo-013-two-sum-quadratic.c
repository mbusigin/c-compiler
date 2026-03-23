/* Expected output:
1 4
*/
#include <stdio.h>

static int two_sum(const int *a, int n, int target, int *out_i, int *out_j) {
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            if (a[i] + a[j] == target) {
                *out_i = i;
                *out_j = j;
                return 1;
            }
        }
    }
    return 0;
}

int main(void) {
    int i, j;
    int a[6] = {4, 9, 1, 7, 5, 3};

    if (two_sum(a, 6, 14, &i, &j)) {
        printf("%d %d\n", i, j);
    }
    return 0;
}
