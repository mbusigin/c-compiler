/* Expected output:
1 2 3
*/
#include <stdio.h>

enum { A = 1, B, C = A + B };

int main(void) {
    printf("%d %d %d\n", A, B, C);
    return 0;
}
