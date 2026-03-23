/* Expected output:
7
*/
#include <stdio.h>

static int max2(int a, int b) {
    return a > b ? a : b;
}

static int knapsack(const int *w, const int *v, int n, int cap) {
    int i, c;
    int dp[8];

    for (c = 0; c <= cap; c++) {
        dp[c] = 0;
    }
    for (i = 0; i < n; i++) {
        for (c = cap; c >= w[i]; c--) {
            dp[c] = max2(dp[c], dp[c - w[i]] + v[i]);
        }
    }
    return dp[cap];
}

int main(void) {
    int w[4] = {2, 1, 3, 2};
    int v[4] = {3, 2, 4, 2};

    printf("%d\n", knapsack(w, v, 4, 5));
    return 0;
}
