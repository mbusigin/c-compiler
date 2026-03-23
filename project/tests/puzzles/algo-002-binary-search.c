/* Expected output:
3 -1
*/
#include <stdio.h>

static int binary_search(const int *a, int n, int key) {
    int lo = 0;
    int hi = n - 1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == key) {
            return mid;
        }
        if (a[mid] < key) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

int main(void) {
    int a[6] = {2, 4, 6, 8, 10, 12};

    printf("%d %d\n", binary_search(a, 6, 8), binary_search(a, 6, 7));
    return 0;
}
