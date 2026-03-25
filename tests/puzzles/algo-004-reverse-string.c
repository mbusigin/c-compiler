/* Expected output:
desserts
*/
#include <stdio.h>

static int str_len(const char *s) {
    int n = 0;

    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static void reverse(char *s) {
    int i = 0;
    int j = str_len(s) - 1;

    while (i < j) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++;
        j--;
    }
}

int main(void) {
    char s[] = "stressed";

    reverse(s);
    printf("%s\n", s);
    return 0;
}
