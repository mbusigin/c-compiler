/* Should compile successfully and print:
123 456
*/
#include <stdio.h>

static int f0(void) { return 123; }
static int f1(void) { return 456; }

int (*(*pick(int x))(void))(void);

static int (*ret0(void))(void) { return f0; }
static int (*ret1(void))(void) { return f1; }

int (*(*pick(int x))(void))(void) {
    return x ? ret1 : ret0;
}

int main(void) {
    printf("%d %d\n", pick(0)()(), pick(1)()());
    return 0;
}
