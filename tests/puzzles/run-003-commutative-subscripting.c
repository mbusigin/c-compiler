/* Expected output:
c c
*/
#include <stdio.h>

int main(void) {
    char s[] = "abcdef";
    printf("%c %c\n", 2[s], s[2]);
    return 0;
}
