/* Expected output:
1 2 3 4 5 6
*/
#include <stdio.h>

static void insertion_sort(int *a, int n) {
    int i;

    for (i = 1; i < n; i++) {
        int x = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > x) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = x;
    }
}

int main(void) {
    int i;
    int a[6] = {1, 2, 4, 3, 5, 6};

    insertion_sort(a, 6);
    for (i = 0; i < 6; i++) {
        printf("%d%c", a[i], i == 5 ? '\n' : ' ');
    }
    return 0;
}
