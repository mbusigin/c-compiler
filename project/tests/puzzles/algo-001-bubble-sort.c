/* Expected output:
1 2 3 4 5
*/
#include <stdio.h>

static void bubble_sort(int *a, int n) {
    int i, j;

    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - 1 - i; j++) {
            if (a[j] > a[j + 1]) {
                int tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }
}

int main(void) {
    int i;
    int a[5] = {5, 1, 4, 2, 3};

    bubble_sort(a, 5);
    for (i = 0; i < 5; i++) {
        printf("%d%c", a[i], i == 4 ? '\n' : ' ');
    }
    return 0;
}
