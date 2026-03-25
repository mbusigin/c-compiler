/* Expected output:
1 1 1 1 1
*/
#include <stdio.h>

static void dfs(int n, const int g[5][5], int v, int *seen) {
    int to;

    (void)n;
    seen[v] = 1;
    for (to = 0; to < 5; to++) {
        if (g[v][to] && !seen[to]) {
            dfs(5, g, to, seen);
        }
    }
}

int main(void) {
    int i;
    int seen[5] = {0, 0, 0, 0, 0};
    int g[5][5] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 0, 0},
        {0, 1, 0, 0, 1},
        {0, 0, 0, 1, 0}
    };

    dfs(5, g, 0, seen);
    for (i = 0; i < 5; i++) {
        printf("%d%c", seen[i], i == 4 ? '\n' : ' ');
    }
    return 0;
}
