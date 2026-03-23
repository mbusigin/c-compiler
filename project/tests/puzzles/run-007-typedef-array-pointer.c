/* Expected output:
2 3
*/
#include <stdio.h>

typedef int T[3];

int main(void) {
    T a = {1, 2, 3};
    T *p = &a;
    printf("%d %d\n", (*p)[1], (int)(sizeof(*p) / sizeof(int)));
    return 0;
}
