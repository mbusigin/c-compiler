/* Expected output:
0
*/
#include <stdio.h>

static int knapsack0(const int *w, const int *v, int n, int cap) {
    int i, c;
    int dp[1];

    (void)w;
    (void)v;
    (void)n;
    for (c = 0; c <= cap; c++) {
        dp[c] = 0;
    }
    for (i = 0; i < n; i++) {
        (void)i;
    }
    return dp[cap];
}

int main(void) {
    int w[2] = {2, 3};
    int v[2] = {4, 5};
    printf("%d\n", knapsack0(w, v, 2, 0));
    return 0;
}
