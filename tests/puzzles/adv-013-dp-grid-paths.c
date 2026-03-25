/* Expected output:
10
*/
#include <stdio.h>

static int paths3x4(void) {
    int r, c;
    int dp[3][4];

    for (r = 0; r < 3; r++) {
        for (c = 0; c < 4; c++) {
            if (r == 0 || c == 0) {
                dp[r][c] = 1;
            } else {
                dp[r][c] = dp[r - 1][c] + dp[r][c - 1];
            }
        }
    }
    return dp[2][3];
}

int main(void) {
    printf("%d\n", paths3x4());
    return 0;
}
