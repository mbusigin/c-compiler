/* Expected output:
1 3
*/
#include <stdio.h>

static int first_index(const int *a, int n, int key) {
    int i;

    for (i = 0; i < n; i++) {
        if (a[i] == key) {
            return i;
        }
    }
    return -1;
}

int main(void) {
    int a[6] = {5, 7, 7, 8, 8, 10};
    printf("%d %d\n", first_index(a, 6, 7), first_index(a, 6, 8));
    return 0;
}
