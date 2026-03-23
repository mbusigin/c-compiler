/* Expected output:
1 1 1
*/
#include <stdio.h>

enum { A = 2, B = A * 3 };

int main(void) {
    printf("%d %d %d\n",
           B == 6,
           sizeof(int[3]) == 3 * sizeof(int),
           _Alignof(int) >= 1);
    return 0;
}
