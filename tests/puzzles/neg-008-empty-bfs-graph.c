/* Expected output:
0 -1 -1
*/
#include <stdio.h>

static void bfs_dist(const int g[3][3], int start, int *dist) {
    int q[3];
    int head = 0;
    int tail = 0;
    int i;

    for (i = 0; i < 3; i++) {
        dist[i] = -1;
    }
    dist[start] = 0;
    q[tail++] = start;
    while (head < tail) {
        int v = q[head++];
        for (i = 0; i < 3; i++) {
            if (g[v][i] && dist[i] == -1) {
                dist[i] = dist[v] + 1;
                q[tail++] = i;
            }
        }
    }
}

int main(void) {
    int dist[3];
    int g[3][3] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    bfs_dist(g, 0, dist);
    printf("%d %d %d\n", dist[0], dist[1], dist[2]);
    return 0;
}
