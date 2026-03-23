/* Expected output:
1 1 2
*/
#include <stdio.h>

static int fib_ok(int n) {
    if (n <= 1) {
        return 1;
    }
    return fib_ok(n - 1) + fib_ok(n - 2);
}

int main(void) {
    printf("%d %d %d\n", fib_ok(0), fib_ok(1), fib_ok(2));
    return 0;
}
