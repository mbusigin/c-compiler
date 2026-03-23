/* Expected output:
30
*/
#include <stdio.h>

int main(void) {
    int i, j;
    int sum = 0;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            sum += i + j;
        }
    }
    printf("%d\n", sum);
    return 0;
}
