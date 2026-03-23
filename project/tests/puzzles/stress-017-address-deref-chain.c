/* Expected output:
55
*/
#include <stdio.h>

int main(void) {
    int x = 55;
    int *p = &x;
    int **pp = &p;

    printf("%d\n", **pp);
    return 0;
}
