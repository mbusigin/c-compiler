/* Expected output:
13
*/
#include <stdio.h>

static int climb(int n) {
    int i;
    int dp[8];

    dp[0] = 1;
    dp[1] = 1;
    for (i = 2; i <= n; i++) {
        dp[i] = dp[i - 1] + dp[i - 2];
    }
    return dp[n];
}

int main(void) {
    printf("%d\n", climb(6));
    return 0;
}
