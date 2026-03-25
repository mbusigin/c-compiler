/* Expected output:
16
*/
#include <stdio.h>

int main(void) {
    int i;
    int sum = 0;

    for (i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            continue;
        }
        if (i == 9) {
            break;
        }
        sum += i;
    }
    printf("%d\n", sum);
    return 0;
}
