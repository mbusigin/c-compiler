/* Expected output:
3
*/
#include <stdio.h>

static int min2(int a, int b) {
    return a < b ? a : b;
}

static int coin_change(const int *coins, int n, int amount) {
    int i, j;
    int dp[12];

    dp[0] = 0;
    for (i = 1; i <= amount; i++) {
        dp[i] = 1000;
    }
    for (i = 0; i < n; i++) {
        for (j = coins[i]; j <= amount; j++) {
            dp[j] = min2(dp[j], dp[j - coins[i]] + 1);
        }
    }
    return dp[amount];
}

int main(void) {
    int coins[3] = {1, 3, 4};

    printf("%d\n", coin_change(coins, 3, 10));
    return 0;
}
