/* Undefined behavior.
   A compiler may warn about unsequenced modification/access to x. */
#include <stdio.h>

int main(void) {
    int x = 1;
    printf("%d %d %d\n", x, x++, ++x);
    return 0;
}
