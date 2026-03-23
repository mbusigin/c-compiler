/* Expected output:
13
*/
#include <stdio.h>

static int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    printf("%d\n", fib(7));
    return 0;
}
