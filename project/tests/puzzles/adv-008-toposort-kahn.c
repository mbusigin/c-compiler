/* Expected output:
0 1 2 3 4 5
*/
#include <stdio.h>

static void topo(const int g[6][6], int *out) {
    int indeg[6] = {0, 0, 0, 0, 0, 0};
    int q[6];
    int head = 0;
    int tail = 0;
    int i, j, k = 0;

    for (i = 0; i < 6; i++) {
        for (j = 0; j < 6; j++) {
            indeg[j] += g[i][j];
        }
    }
    for (i = 0; i < 6; i++) {
        if (indeg[i] == 0) {
            q[tail++] = i;
        }
    }
    while (head < tail) {
        int v = q[head++];
        out[k++] = v;
        for (j = 0; j < 6; j++) {
            if (g[v][j]) {
                indeg[j]--;
                if (indeg[j] == 0) {
                    q[tail++] = j;
                }
            }
        }
    }
}

int main(void) {
    int i;
    int out[6];
    int g[6][6] = {
        {0, 1, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0},
        {0, 0, 0, 1, 1, 0},
        {0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0}
    };

    topo(g, out);
    for (i = 0; i < 6; i++) {
        printf("%d%c", out[i], i == 5 ? '\n' : ' ');
    }
    return 0;
}
