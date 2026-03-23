/* Expected output:
4
*/
#include <stdio.h>

static int lis(const int *a, int n) {
    int i, j;
    int best = 0;
    int dp[8];

    for (i = 0; i < n; i++) {
        dp[i] = 1;
        for (j = 0; j < i; j++) {
            if (a[j] < a[i] && dp[j] + 1 > dp[i]) {
                dp[i] = dp[j] + 1;
            }
        }
        if (dp[i] > best) {
            best = dp[i];
        }
    }
    return best;
}

int main(void) {
    int a[8] = {10, 9, 2, 5, 3, 7, 101, 18};

    printf("%d\n", lis(a, 8));
    return 0;
}
