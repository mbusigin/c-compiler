/* Expected output:
3 2 0
*/
#include <stdio.h>

static int starts_with(const char *s, const char *prefix) {
    int i = 0;

    while (prefix[i] != '\0') {
        if (s[i] != prefix[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

static int count_prefix(const char *const *words, int n, const char *prefix) {
    int i;
    int count = 0;

    for (i = 0; i < n; i++) {
        if (starts_with(words[i], prefix)) {
            count++;
        }
    }
    return count;
}

int main(void) {
    const char *words[5] = {"cat", "car", "dog", "cart", "dot"};

    printf("%d %d %d\n",
           count_prefix(words, 5, "ca"),
           count_prefix(words, 5, "do"),
           count_prefix(words, 5, "z"));
    return 0;
}
