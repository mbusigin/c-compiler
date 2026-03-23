/* Undefined behavior.
   A compiler may warn about multiple unsequenced modifications of p. */
#include <stdio.h>

int main(void) {
    int a[] = {10, 20, 30, 40};
    int *p = a;
    printf("%d\n", *p++ + *++p);
    return 0;
}
