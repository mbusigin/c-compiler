/* Expected output:
0 1 1 2 3
*/
#include <stdio.h>

static void bfs_dist(const int g[5][5], int start, int *dist) {
    int q[8];
    int head = 0;
    int tail = 0;
    int i;

    for (i = 0; i < 5; i++) {
        dist[i] = -1;
    }
    dist[start] = 0;
    q[tail++] = start;
    while (head < tail) {
        int v = q[head++];
        for (i = 0; i < 5; i++) {
            if (g[v][i] && dist[i] == -1) {
                dist[i] = dist[v] + 1;
                q[tail++] = i;
            }
        }
    }
}

int main(void) {
    int i;
    int dist[5];
    int g[5][5] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 0, 0},
        {0, 1, 0, 0, 1},
        {0, 0, 0, 1, 0}
    };

    bfs_dist(g, 0, dist);
    for (i = 0; i < 5; i++) {
        printf("%d%c", dist[i], i == 4 ? '\n' : ' ');
    }
    return 0;
}
