/* Expected output:
19 22 43 50
*/
#include <stdio.h>

static void mul2(const int a[2][2], const int b[2][2], int out[2][2]) {
    int i, j, k;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            out[i][j] = 0;
            for (k = 0; k < 2; k++) {
                out[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

int main(void) {
    int i, j;
    int a[2][2] = {{1, 2}, {3, 4}};
    int b[2][2] = {{5, 6}, {7, 8}};
    int out[2][2];

    mul2(a, b, out);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            int last = i == 1 && j == 1;
            printf("%d%c", out[i][j], last ? '\n' : ' ');
        }
    }
    return 0;
}
