/* Expected output:
1 1 2 3 3
*/
#include <stdio.h>

static void sort(int *a, int n) {
    int i, j;

    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - 1 - i; j++) {
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
        }
    }
}

int main(void) {
    int i;
    int a[5] = {3, 1, 3, 2, 1};

    sort(a, 5);
    for (i = 0; i < 5; i++) {
        printf("%d%c", a[i], i == 4 ? '\n' : ' ');
    }
    return 0;
}
